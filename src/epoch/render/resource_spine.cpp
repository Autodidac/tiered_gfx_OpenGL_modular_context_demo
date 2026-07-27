#include "epoch/render/resource_spine.hpp"
#include "epoch/render/gl/gl_api.hpp"

#include <sstream>
#include <stdexcept>

namespace epoch::render {
namespace {
std::string normalized_key(const std::filesystem::path& path) {
    return path.lexically_normal().generic_string();
}
}

ResourceSpine::ResourceSpine(std::filesystem::path asset_root) : asset_root_{std::move(asset_root)} {
    while (gl::GetError() != GL_NO_ERROR) {}
    GLfloat maximum = 1.0f;
    gl::GetFloatv(gl::max_texture_max_anisotropy_ext, &maximum);
    if (gl::GetError() == GL_NO_ERROR && maximum >= 1.0f) max_anisotropy_ = maximum;
}

MeshHandle ResourceSpine::create_mesh(gl::MeshData data) {
    meshes_.push_back(std::make_unique<gl::Mesh>(data.vertices, data.indices));
    return MeshHandle{static_cast<std::uint32_t>(meshes_.size())};
}

MeshHandle ResourceSpine::load_obj_mesh(const std::filesystem::path& relative_path) {
    const std::string key = normalized_key(relative_path);
    if (const auto found = mesh_cache_.find(key); found != mesh_cache_.end()) return found->second;
    const MeshHandle handle = create_mesh(gl::load_obj(asset_root_ / relative_path));
    mesh_cache_.emplace(key, handle);
    return handle;
}

TextureHandle ResourceSpine::load_texture(const std::filesystem::path& relative_path, gl::ColorSpace color_space,
                                          bool flip_y) {
    const std::string key = normalized_key(relative_path) + (color_space == gl::ColorSpace::srgb ? "|s" : "|l")
        + (flip_y ? "|f" : "|n");
    if (const auto found = texture_cache_.find(key); found != texture_cache_.end()) return found->second;
    textures_.push_back(std::make_unique<gl::Texture2D>(asset_root_ / relative_path, color_space, max_anisotropy_, flip_y));
    const TextureHandle handle{static_cast<std::uint32_t>(textures_.size())};
    texture_cache_.emplace(key, handle);
    return handle;
}

CubeTextureHandle ResourceSpine::load_cubemap(const std::array<std::filesystem::path, 6>& relative_faces,
                                               gl::ColorSpace color_space, bool flip_y) {
    std::ostringstream key_stream;
    for (const auto& face : relative_faces) key_stream << normalized_key(face) << ';';
    key_stream << (color_space == gl::ColorSpace::srgb ? 's' : 'l') << (flip_y ? 'f' : 'n');
    const std::string key = key_stream.str();
    if (const auto found = cubemap_cache_.find(key); found != cubemap_cache_.end()) return found->second;
    std::array<std::filesystem::path, 6> absolute{};
    for (std::size_t index = 0; index < absolute.size(); ++index) absolute[index] = asset_root_ / relative_faces[index];
    cubemaps_.push_back(std::make_unique<gl::TextureCube>(absolute, color_space, flip_y));
    const CubeTextureHandle handle{static_cast<std::uint32_t>(cubemaps_.size())};
    cubemap_cache_.emplace(key, handle);
    return handle;
}

const gl::Mesh& ResourceSpine::mesh(MeshHandle handle) const {
    if (!handle || handle.value > meshes_.size()) throw std::out_of_range("Invalid mesh handle");
    return *meshes_[handle.value - 1];
}

const gl::Texture2D& ResourceSpine::texture(TextureHandle handle) const {
    if (!handle || handle.value > textures_.size()) throw std::out_of_range("Invalid texture handle");
    return *textures_[handle.value - 1];
}

const gl::TextureCube& ResourceSpine::cubemap(CubeTextureHandle handle) const {
    if (!handle || handle.value > cubemaps_.size()) throw std::out_of_range("Invalid cubemap handle");
    return *cubemaps_[handle.value - 1];
}

} // namespace epoch::render
