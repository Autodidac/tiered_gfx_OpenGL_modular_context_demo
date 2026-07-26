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

class WaterSimulationTechnique {
public:
    WaterSimulationTechnique(const std::filesystem::path& shader_root, ResourceSpine& resources);
    ~WaterSimulationTechnique();
    WaterSimulationTechnique(const WaterSimulationTechnique&) = delete;
    WaterSimulationTechnique& operator=(const WaterSimulationTechnique&) = delete;
    void capture_scene(int width, int height);
    void render(const TechniqueContext& frame, const scene::SceneSpine& scene,
                const ResourceSpine& resources) const;

private:
    gl::ShaderProgram shader_;
    MeshHandle grid_mesh_{};
    TextureHandle normal_map_{};
    TextureHandle foam_map_{};
    GLuint scene_copy_{};
    GLuint scene_depth_{};
    int scene_copy_width_{};
    int scene_copy_height_{};
};

} // namespace epoch::render::techniques
