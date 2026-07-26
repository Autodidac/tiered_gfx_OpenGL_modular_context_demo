#include "epoch/render/techniques/sky_environment.hpp"

namespace epoch::render::techniques {

SkyEnvironmentTechnique::SkyEnvironmentTechnique(const std::filesystem::path& root, MeshHandle sky_mesh)
    : shader_{root / "sky/sky.vert", root / "sky/sky.frag"}, sky_mesh_{sky_mesh} {}

void SkyEnvironmentTechnique::render(const TechniqueContext& frame, const scene::SceneSpine& scene, const ResourceSpine& resources) const {
    gl::DepthFunc(GL_LEQUAL); gl::DepthMask(GL_FALSE); gl::Disable(GL_CULL_FACE);
    shader_.bind();
    shader_.set("uView", frame.view);
    shader_.set("uProjection", frame.projection);
    shader_.set("uEnvironment", 0);
    shader_.set("uExposureScale", 0.42f);
    shader_.set("uSunColor", scene.sun.color);
    shader_.set("uSunDirection", scene.sun.direction);
    resources.cubemap(scene.environment).bind(0);
    resources.mesh(sky_mesh_).draw();
    gl::DepthMask(GL_TRUE); gl::DepthFunc(GL_LESS); gl::Enable(GL_CULL_FACE); gl::CullFace(GL_BACK);
}

} // namespace epoch::render::techniques
