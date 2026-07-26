#pragma once


#include <array>
#include <filesystem>
#include "epoch/render/gl/shader_program.hpp"
#include "epoch/render/resource_spine.hpp"
#include "epoch/render/techniques/shadow_mapping.hpp"

#if defined(EPOCH_NO_MODULES)
#include "epoch/compat/epoch.render.scene.hpp"
#else
import epoch.render.scene;
#endif
#if defined(EPOCH_NO_MODULES)
#include "epoch/compat/epoch.render.technique.context.hpp"
#else
import epoch.render.technique.context;
#endif

namespace epoch::render::techniques {

class TessellationTechnique {
public:
    TessellationTechnique(const std::filesystem::path& shader_root, ResourceSpine& resources, MeshHandle patch_mesh);
    void render(const TechniqueContext& frame, const scene::SceneSpine& scene,
                const ResourceSpine& resources, const ShadowMappingTechnique& shadows) const;

private:
    void bind_common(const gl::ShaderProgram& shader, const TechniqueContext& frame,
                     const scene::SceneSpine& scene, const ResourceSpine& resources,
                     const ShadowMappingTechnique& shadows) const;

    gl::ShaderProgram tessellation_shader_;
    gl::ShaderProgram tier0_shader_;
    MeshHandle patch_mesh_{};
    TextureHandle height_map_{};
    TextureHandle splat_map_{};
    TextureHandle detail_height_map_{};
    std::array<TextureHandle,4> albedo_maps_{};
    std::array<TextureHandle,4> normal_maps_{};
    std::array<TextureHandle,4> orm_maps_{};
};

} // namespace epoch::render::techniques
