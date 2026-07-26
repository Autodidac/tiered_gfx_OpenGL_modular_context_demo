#pragma once

#include <filesystem>
#include <vector>

#include "epoch/render/gl/image.hpp"
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

// Tier-0 foliage path: camera-facing quads expanded in the vertex shader.
// Instance placement is rebuilt from the editable paint mask, terrain height,
// and solid-object exclusions. No geometry shader or compute shader is needed.
class BillboardFoliageTechnique {
public:
    BillboardFoliageTechnique(const std::filesystem::path& shader_root,
                              ResourceSpine& resources,
                              MeshHandle unused_quad);
    BillboardFoliageTechnique(const BillboardFoliageTechnique&) = delete;
    BillboardFoliageTechnique& operator=(const BillboardFoliageTechnique&) = delete;
    ~BillboardFoliageTechnique();

    void render(const TechniqueContext& frame,
                const scene::SceneSpine& scene,
                const ResourceSpine& resources) const;

private:
    struct Instance {
        // Template instances use normalized region coordinates in X/Z. The
        // dynamic render stream stores final world coordinates in this field.
        math::Vec3 center{};
        math::Vec2 size{};
        float phase{};
        float stiffness{1.0f};
    };

    [[nodiscard]] float terrain_height(math::Vec2 world_xz,
                                       const scene::TerrainSurface& terrain,
                                       bool detailed,
                                       float tessellation_level) const noexcept;
    [[nodiscard]] bool excluded_by_solid(math::Vec3 world_position,
                                         const scene::SceneSpine& scene) const noexcept;

    gl::ShaderProgram shader_;
    TextureHandle albedo_{};
    TextureHandle opacity_{};
    gl::Image terrain_height_image_{};
    gl::Image detail_height_image_{};
    std::vector<Instance> templates_;
    mutable std::vector<Instance> visible_instances_;
    GLuint vao_{};
    GLuint vertex_buffer_{};
    GLuint index_buffer_{};
    GLuint instance_buffer_{};
};

} // namespace epoch::render::techniques
