#pragma once


#include <cstdint>
#include <filesystem>
#include <vector>

#include "epoch/render/gl/mesh.hpp"
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

class ShadowMappingTechnique;

class ClothSimulationTechnique {
public:
    ClothSimulationTechnique(const std::filesystem::path& shader_root, ResourceSpine& resources);
    ClothSimulationTechnique(const ClothSimulationTechnique&) = delete;
    ClothSimulationTechnique& operator=(const ClothSimulationTechnique&) = delete;
    ~ClothSimulationTechnique();

    void update(const scene::SceneSpine& scene, float delta_seconds, float elapsed_seconds,
                const context::RuntimeControls& controls);
    void render_shadow(const TechniqueContext& frame, const scene::SceneSpine& scene) const;
    void render(const TechniqueContext& frame, const scene::SceneSpine& scene,
                const ResourceSpine& resources, const ShadowMappingTechnique& shadows) const;

private:
    struct Particle {
        math::Vec3 position{};
        math::Vec3 previous{};
        bool pinned{};
    };

    struct RuntimeCloth {
        std::size_t descriptor_index{};
        std::uint32_t columns{};
        std::uint32_t rows{};
        float rest_x{};
        float rest_y{};
        float accumulator{};
        math::Vec3 descriptor_origin{};
        math::Vec3 descriptor_right_axis{};
        math::Vec3 descriptor_down_axis{};
        float descriptor_width{};
        float descriptor_height{};
        std::vector<Particle> particles;
        std::vector<gl::Vertex> vertices;
        std::vector<std::uint32_t> indices;
        GLuint vao{};
        GLuint vbo{};
        GLuint ebo{};
    };

    void synchronize(const scene::SceneSpine& scene);
    static void initialize(RuntimeCloth& runtime, const scene::ClothObject& descriptor);
    static void simulate(RuntimeCloth& runtime, const scene::ClothObject& descriptor,
                         float delta_seconds, float elapsed_seconds, float wind_strength);
    static void upload(RuntimeCloth& runtime, const scene::ClothObject& descriptor);
    static void destroy(RuntimeCloth& runtime) noexcept;

    gl::ShaderProgram shader_;
    gl::ShaderProgram shadow_shader_;
    TextureHandle fallback_albedo_{};
    TextureHandle fallback_normal_{};
    TextureHandle fallback_orm_{};
    std::vector<RuntimeCloth> cloths_;
};

} // namespace epoch::render::techniques
