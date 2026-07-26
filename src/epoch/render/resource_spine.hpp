#pragma once

#include <array>
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "epoch/render/gl/mesh.hpp"
#include "epoch/render/gl/texture.hpp"

#if defined(EPOCH_NO_MODULES)
#include "epoch/compat/epoch.render.types.hpp"
#else
import epoch.render.types;
#endif

namespace epoch::render {

class ResourceSpine {
public:
    explicit ResourceSpine(std::filesystem::path asset_root);

    [[nodiscard]] MeshHandle create_mesh(gl::MeshData data);
    [[nodiscard]] MeshHandle load_obj_mesh(const std::filesystem::path& relative_path);
    [[nodiscard]] TextureHandle load_texture(const std::filesystem::path& relative_path, gl::ColorSpace color_space,
                                             bool flip_y = true);
    [[nodiscard]] CubeTextureHandle load_cubemap(const std::array<std::filesystem::path, 6>& relative_faces,
                                                 gl::ColorSpace color_space, bool flip_y = false);

    [[nodiscard]] const gl::Mesh& mesh(MeshHandle handle) const;
    [[nodiscard]] const gl::Texture2D& texture(TextureHandle handle) const;
    [[nodiscard]] const gl::TextureCube& cubemap(CubeTextureHandle handle) const;
    [[nodiscard]] const std::filesystem::path& asset_root() const noexcept { return asset_root_; }
    [[nodiscard]] float max_anisotropy() const noexcept { return max_anisotropy_; }

private:
    std::filesystem::path asset_root_;
    float max_anisotropy_{1.0f};
    std::vector<std::unique_ptr<gl::Mesh>> meshes_;
    std::vector<std::unique_ptr<gl::Texture2D>> textures_;
    std::vector<std::unique_ptr<gl::TextureCube>> cubemaps_;
    std::unordered_map<std::string, MeshHandle> mesh_cache_;
    std::unordered_map<std::string, TextureHandle> texture_cache_;
    std::unordered_map<std::string, CubeTextureHandle> cubemap_cache_;
};

} // namespace epoch::render
