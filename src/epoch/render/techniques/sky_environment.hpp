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

class SkyEnvironmentTechnique {
public:
    SkyEnvironmentTechnique(const std::filesystem::path& shader_root, MeshHandle sky_mesh);
    void render(const TechniqueContext& frame, const scene::SceneSpine& scene, const ResourceSpine& resources) const;

private:
    gl::ShaderProgram shader_;
    MeshHandle sky_mesh_{};
};

} // namespace epoch::render::techniques
