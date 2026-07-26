#include "epoch/render/techniques/instanced_props.hpp"
#include <array>
#include <cmath>

namespace epoch::render::techniques {

InstancedPropsTechnique::InstancedPropsTechnique(const std::filesystem::path& root, ResourceSpine& resources, MeshHandle mesh)
    : shader_{root / "instancing/instanced.vert", root / "instancing/instanced.frag"},
      mesh_{mesh},
      albedo_{resources.load_texture("default_pack/textures/materials/wood/base_color.png", gl::ColorSpace::srgb)},
      orm_{resources.load_texture("default_pack/textures/materials/wood/orm.png", gl::ColorSpace::linear)},
      index_count_{static_cast<GLuint>(resources.mesh(mesh_).index_count())} {
    const DrawElementsIndirectCommand command{
        .count = index_count_,
        .instance_count = 12,
        .first_index = 0,
        .base_vertex = 0,
        .base_instance = 0
    };
    gl::GenBuffers(1, &indirect_buffer_);
    gl::BindBuffer(gl::draw_indirect_buffer, indirect_buffer_);
    gl::BufferData(gl::draw_indirect_buffer, sizeof(command), &command, gl::static_draw);
    gl::BindBuffer(gl::draw_indirect_buffer, 0);
}

InstancedPropsTechnique::~InstancedPropsTechnique() {
    if (indirect_buffer_) gl::DeleteBuffers(1, &indirect_buffer_);
}

void InstancedPropsTechnique::render(const TechniqueContext& frame, const scene::SceneSpine& scene,
                                     const ResourceSpine& resources) const {
    if (!frame.controls.instancing) return;

    std::array<const scene::RenderObject*, 12> ordered{};
    for (const auto& object : scene.objects) {
        if (object.editor_binding != scene::EditorBindingKind::instanced_prop
            || object.editor_binding_index >= ordered.size()) continue;
        ordered[object.editor_binding_index] = &object;
    }

    std::array<math::Mat4, 12> models{};
    std::size_t active_count{};
    for (std::size_t index = 0; index < ordered.size(); ++index) {
        const auto* object = ordered[index];
        if (!object || !object->visible || object->editor_deleted) continue;
        auto transform = object->transform;
        transform.rotation.y += std::sin(frame.elapsed_seconds * frame.controls.animation_speed * 0.18f
            + static_cast<float>(index)) * 0.025f;
        models[active_count++] = transform.matrix();
    }
    if (active_count == 0u) return;

    shader_.bind();
    shader_.set("uViewProjection", frame.view_projection);
    shader_.set_matrix_array("uInstanceModels[0]", std::span<const math::Mat4>{models.data(), active_count});
    shader_.set("uCamera", frame.camera_position);
    shader_.set("uSunDirection", scene.sun.direction);
    shader_.set("uSunColor", scene.sun.color * frame.controls.sun_intensity);
    shader_.set("uAlbedo", 0);
    shader_.set("uOrm", 1);
    shader_.set("uEnvironment", 2);
    resources.texture(albedo_).bind(0);
    resources.texture(orm_).bind(1);
    resources.cubemap(scene.environment).bind(2);

    if (frame.controls.indirect_draw && !frame.controls.tier0_mobile_profile) {
        const DrawElementsIndirectCommand command{
            .count = index_count_,
            .instance_count = static_cast<GLuint>(active_count),
            .first_index = 0,
            .base_vertex = 0,
            .base_instance = 0
        };
        gl::BindBuffer(gl::draw_indirect_buffer, indirect_buffer_);
        gl::BufferSubData(gl::draw_indirect_buffer, 0, sizeof(command), &command);
        resources.mesh(mesh_).draw_indirect();
        gl::BindBuffer(gl::draw_indirect_buffer, 0);
    } else {
        resources.mesh(mesh_).draw_instanced(static_cast<GLsizei>(active_count));
    }
}


} // namespace epoch::render::techniques
