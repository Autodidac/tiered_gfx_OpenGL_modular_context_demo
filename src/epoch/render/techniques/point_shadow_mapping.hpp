#pragma once


#include <filesystem>

#include "epoch/render/gl/shader_program.hpp"
#include "epoch/render/resource_spine.hpp"

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

// One low-resolution six-pass cubemap shadow is intentionally used for Tier 0.
// It requires only standard framebuffer/cubemap functionality and avoids geometry
// shaders, layered rendering, bindless residency, or vendor extensions.
class PointShadowMappingTechnique {
public:
    explicit PointShadowMappingTechnique(const std::filesystem::path& shader_root, int size = 512);
    PointShadowMappingTechnique(const PointShadowMappingTechnique&) = delete;
    PointShadowMappingTechnique& operator=(const PointShadowMappingTechnique&) = delete;
    ~PointShadowMappingTechnique();

    void render(const TechniqueContext& frame, const scene::SceneSpine& scene,
                const ResourceSpine& resources) const;
    void bind_depth(int unit) const noexcept;

    [[nodiscard]] int light_index() const noexcept { return light_index_; }
    [[nodiscard]] float far_plane(const scene::SceneSpine& scene) const noexcept;

private:
    gl::ShaderProgram shader_;
    GLuint framebuffer_{};
    GLuint depth_cubemap_{};
    int size_{512};
    int light_index_{};
};

} // namespace epoch::render::techniques
