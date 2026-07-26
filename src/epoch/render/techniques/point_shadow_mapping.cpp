#include "epoch/render/techniques/point_shadow_mapping.hpp"

#include <algorithm>
#include <array>
#include <stdexcept>

namespace epoch::render::techniques {

PointShadowMappingTechnique::PointShadowMappingTechnique(
    const std::filesystem::path& root, int size)
    : shader_{root / "point_shadow/point_shadow.vert", root / "point_shadow/point_shadow.frag"},
      size_{std::max(size, 128)} {
    gl::GenFramebuffers(1, &framebuffer_);
    gl::GenTextures(1, &depth_cubemap_);
    gl::BindTexture(gl::texture_cube_map, depth_cubemap_);
    for (int face = 0; face < 6; ++face) {
        gl::TexImage2D(gl::texture_cube_map_positive_x + face, 0,
                     static_cast<GLint>(gl::depth_component24), size_, size_, 0,
                     GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr);
    }
    gl::TexParameteri(gl::texture_cube_map, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    gl::TexParameteri(gl::texture_cube_map, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    gl::TexParameteri(gl::texture_cube_map, GL_TEXTURE_WRAP_S, gl::clamp_to_edge);
    gl::TexParameteri(gl::texture_cube_map, GL_TEXTURE_WRAP_T, gl::clamp_to_edge);
    gl::TexParameteri(gl::texture_cube_map, gl::texture_wrap_r, gl::clamp_to_edge);

    gl::BindFramebuffer(gl::framebuffer, framebuffer_);
    gl::FramebufferTexture2D(gl::framebuffer, gl::depth_attachment,
                             gl::texture_cube_map_positive_x, depth_cubemap_, 0);
    gl::DrawBuffer(GL_NONE);
    gl::ReadBuffer(GL_NONE);
    if (gl::CheckFramebufferStatus(gl::framebuffer) != gl::framebuffer_complete) {
        throw std::runtime_error("Point-shadow cubemap framebuffer is incomplete");
    }
    gl::BindFramebuffer(gl::framebuffer, 0);
}

PointShadowMappingTechnique::~PointShadowMappingTechnique() {
    if (depth_cubemap_) gl::DeleteTextures(1, &depth_cubemap_);
    if (framebuffer_) gl::DeleteFramebuffers(1, &framebuffer_);
}

float PointShadowMappingTechnique::far_plane(const scene::SceneSpine& scene) const noexcept {
    if (scene.point_light_count <= static_cast<std::size_t>(light_index_)) return 1.0f;
    return std::max(scene.point_lights[light_index_].radius, 1.0f);
}

void PointShadowMappingTechnique::render(
    const TechniqueContext& frame, const scene::SceneSpine& scene,
    const ResourceSpine& resources) const {
    if (!frame.controls.shadows || !frame.controls.point_shadows
        || !frame.controls.point_lights
        || scene.point_light_count <= static_cast<std::size_t>(light_index_)) return;

    const auto& light = scene.point_lights[light_index_];
    const float z_far = far_plane(scene);
    const math::Mat4 projection = math::perspective(math::radians(90.0f), 1.0f, 0.10f, z_far);
    const std::array<math::Mat4, 6> views{
        math::look_at(light.position, light.position + math::Vec3{ 1, 0, 0}, {0,-1, 0}),
        math::look_at(light.position, light.position + math::Vec3{-1, 0, 0}, {0,-1, 0}),
        math::look_at(light.position, light.position + math::Vec3{ 0, 1, 0}, {0, 0, 1}),
        math::look_at(light.position, light.position + math::Vec3{ 0,-1, 0}, {0, 0,-1}),
        math::look_at(light.position, light.position + math::Vec3{ 0, 0, 1}, {0,-1, 0}),
        math::look_at(light.position, light.position + math::Vec3{ 0, 0,-1}, {0,-1, 0})
    };

    gl::BindFramebuffer(gl::framebuffer, framebuffer_);
    gl::Viewport(0, 0, size_, size_);
    gl::Enable(GL_DEPTH_TEST);
    gl::Enable(GL_CULL_FACE);
    gl::CullFace(GL_FRONT);
    gl::Enable(GL_POLYGON_OFFSET_FILL);
    gl::PolygonOffset(1.4f, 2.4f);

    shader_.bind();
    shader_.set("uLightPosition", light.position);
    shader_.set("uFarPlane", z_far);
    for (int face = 0; face < 6; ++face) {
        gl::FramebufferTexture2D(gl::framebuffer, gl::depth_attachment,
                                 gl::texture_cube_map_positive_x + face,
                                 depth_cubemap_, 0);
        gl::Clear(GL_DEPTH_BUFFER_BIT);
        shader_.set("uLightViewProjection", projection * views[face]);
        for (const auto& object : scene.objects) {
            if (!object.visible || object.editor_only || !object.casts_shadow
                || object.editor_binding == scene::EditorBindingKind::instanced_prop) continue;
            const auto& material = scene.material(object.material);
            if (scene::has_feature(material.features, scene::MaterialFeature::transmission)) continue;
            shader_.set("uModel", object.transform.matrix());
            resources.mesh(object.mesh).draw();
        }
    }

    gl::Disable(GL_POLYGON_OFFSET_FILL);
    gl::CullFace(GL_BACK);
}

void PointShadowMappingTechnique::bind_depth(int unit) const noexcept {
    gl::ActiveTexture(gl::texture0 + unit);
    gl::BindTexture(gl::texture_cube_map, depth_cubemap_);
}

} // namespace epoch::render::techniques
