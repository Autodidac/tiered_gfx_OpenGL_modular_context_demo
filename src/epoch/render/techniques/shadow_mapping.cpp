#include "epoch/render/techniques/shadow_mapping.hpp"


#if defined(EPOCH_NO_MODULES)
#include "epoch/compat/epoch.render.technique.context.hpp"
#else
import epoch.render.technique.context;
#endif
namespace epoch::render::techniques {

ShadowMappingTechnique::ShadowMappingTechnique(const std::filesystem::path& root)
    : shader_{root / "shadow/shadow.vert", root / "shadow/shadow.frag"} {}

void ShadowMappingTechnique::render(const TechniqueContext& frame, const scene::SceneSpine& scene, const ResourceSpine& resources) const {
    if (!frame.controls.shadows) return;
    target_.bind_for_write();
    if (!frame.controls.directional_light || scene.sun.direction.y >= -0.03f) return;
    gl::Enable(GL_DEPTH_TEST); gl::Enable(GL_CULL_FACE); gl::CullFace(GL_FRONT);
    gl::Enable(GL_POLYGON_OFFSET_FILL); gl::PolygonOffset(2.0f, 4.0f);
    shader_.bind(); shader_.set("uLightViewProjection", frame.light_view_projection);
    for (const auto& object : scene.objects) {
        if (!object.visible || object.editor_only || !object.casts_shadow
                || object.editor_binding == scene::EditorBindingKind::instanced_prop) continue;
        shader_.set("uModel", object.transform.matrix());
        resources.mesh(object.mesh).draw();
    }
    gl::Disable(GL_POLYGON_OFFSET_FILL); gl::CullFace(GL_BACK);
}

} // namespace epoch::render::techniques
