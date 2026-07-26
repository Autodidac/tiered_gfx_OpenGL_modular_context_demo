#include "epoch/render/techniques/tessellation.hpp"

#include <array>
#include <cstdio>

namespace epoch::render::techniques {

TessellationTechnique::TessellationTechnique(const std::filesystem::path& root, ResourceSpine& resources,
                                             MeshHandle patch_mesh)
    : tessellation_shader_{{
          {gl::vertex_shader, root / "tessellation/tess.vert"},
          {gl::tess_control_shader, root / "tessellation/tess.tesc"},
          {gl::tess_evaluation_shader, root / "tessellation/tess.tese"},
          {gl::fragment_shader, root / "tessellation/tess.frag"}}},
      tier0_shader_{root / "tessellation/mobile.vert", root / "tessellation/tess.frag"},
      patch_mesh_{patch_mesh},
      height_map_{resources.load_texture("default_pack/maps/starter_island_height.png", gl::ColorSpace::linear)},
      splat_map_{resources.load_texture("default_pack/maps/starter_island_splat.png", gl::ColorSpace::linear)},
      detail_height_map_{resources.load_texture(
          "default_pack/textures/materials/rock/height.png", gl::ColorSpace::linear)} {
    constexpr std::array names{"field_grass", "dirt", "rock", "sand"};
    for (std::size_t index = 0; index < names.size(); ++index) {
        const std::filesystem::path base = index == 0
            ? std::filesystem::path{"curated/materials"} / names[index]
            : std::filesystem::path{"default_pack/textures/materials"} / names[index];
        albedo_maps_[index] = resources.load_texture(base / "base_color.png", gl::ColorSpace::srgb);
        normal_maps_[index] = resources.load_texture(base / "normal.png", gl::ColorSpace::linear);
        orm_maps_[index] = resources.load_texture(base / "orm.png", gl::ColorSpace::linear);
    }
}

void TessellationTechnique::bind_common(
    const gl::ShaderProgram& shader, const TechniqueContext& frame,
    const scene::SceneSpine& scene, const ResourceSpine& resources,
    const ShadowMappingTechnique& shadows) const {
    shader.bind();
    shader.set("uViewProjection", frame.view_projection);
    shader.set("uLightViewProjection", frame.light_view_projection);
    shader.set("uModel", scene.terrain.transform.matrix());
    shader.set("uCameraPosition", frame.camera_position);
    shader.set("uSunDirection", scene.sun.direction);
    shader.set("uSunColor", scene.sun.color * frame.controls.sun_intensity);
    shader.set("uEnvironmentStrength", frame.controls.environment_strength);
    shader.set("uTessLevel", frame.controls.tessellation_level);
    shader.set("uHeightScale", scene.terrain.height_scale);
    shader.set("uPnTriangles", frame.controls.pn_triangles ? 1 : 0);
    shader.set("uShadowsEnabled", frame.controls.shadows ? 1 : 0);
    shader.set("uFogEnabled", frame.controls.fog ? 1 : 0);
    shader.set("uFogDensity", frame.controls.fog_density);
    shader.set("uFogHeightFalloff", frame.controls.fog_height_falloff);
    shader.set("uHeightMap", 0);
    shader.set("uSplatMap", 1);
    resources.texture(height_map_).bind(0);
    resources.texture(splat_map_).bind(1);
    for (std::size_t index = 0; index < 4; ++index) {
        char name[32]{};
        std::snprintf(name, sizeof(name), "uAlbedoMaps[%zu]", index);
        shader.set(name, static_cast<int>(2 + index));
        resources.texture(albedo_maps_[index]).bind(static_cast<int>(2 + index));
        std::snprintf(name, sizeof(name), "uNormalMaps[%zu]", index);
        shader.set(name, static_cast<int>(6 + index));
        resources.texture(normal_maps_[index]).bind(static_cast<int>(6 + index));
        std::snprintf(name, sizeof(name), "uOrmMaps[%zu]", index);
        shader.set(name, static_cast<int>(10 + index));
        resources.texture(orm_maps_[index]).bind(static_cast<int>(10 + index));
    }
    shader.set("uShadowMap", 14);
    shadows.bind_depth(14);
    shader.set("uDetailHeightMap", 15);
    resources.texture(detail_height_map_).bind(15);
}

void TessellationTechnique::render(const TechniqueContext& frame, const scene::SceneSpine& scene,
                                   const ResourceSpine& resources, const ShadowMappingTechnique& shadows) const {
    if (!scene.terrain.visible) return;
    const bool advanced_tessellation = frame.controls.tessellation;
    const gl::ShaderProgram& shader = advanced_tessellation ? tessellation_shader_ : tier0_shader_;
    bind_common(shader, frame, scene, resources, shadows);

    const auto& mesh = resources.mesh(patch_mesh_);
    if (advanced_tessellation) {
        gl::PatchParameteri(gl::patch_vertices, 3);
        mesh.bind();
        gl::DrawElements(gl::patches, mesh.index_count(), GL_UNSIGNED_INT, nullptr);
    } else {
        mesh.draw();
    }
}

} // namespace epoch::render::techniques
