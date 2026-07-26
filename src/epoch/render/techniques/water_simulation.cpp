#include "epoch/render/techniques/water_simulation.hpp"

#include <algorithm>

namespace epoch::render::techniques {

WaterSimulationTechnique::WaterSimulationTechnique(
    const std::filesystem::path& root, ResourceSpine& resources)
    : shader_{root / "water/water.vert", root / "water/water.frag"},
      grid_mesh_{resources.create_mesh(gl::make_grid_mesh(96, 96, 2.0f, 2.0f, 3.0f))},
      normal_map_{resources.load_texture(
          "curated/water/wave_normal.png", gl::ColorSpace::linear)},
      foam_map_{resources.load_texture(
          "curated/water/foam_noise.png", gl::ColorSpace::linear)} {}

WaterSimulationTechnique::~WaterSimulationTechnique() {
    if (scene_depth_) gl::DeleteTextures(1, &scene_depth_);
    if (scene_copy_) gl::DeleteTextures(1, &scene_copy_);
}

void WaterSimulationTechnique::capture_scene(int width, int height) {
    width = std::max(width, 1);
    height = std::max(height, 1);
    if (!scene_copy_ || !scene_depth_ || width != scene_copy_width_ || height != scene_copy_height_) {
        if (scene_depth_) gl::DeleteTextures(1, &scene_depth_);
        if (scene_copy_) gl::DeleteTextures(1, &scene_copy_);
        scene_copy_width_ = width;
        scene_copy_height_ = height;

        gl::GenTextures(1, &scene_copy_);
        gl::ActiveTexture(gl::texture0 + 3);
        gl::BindTexture(GL_TEXTURE_2D, scene_copy_);
        gl::TexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(gl::rgba16f), width, height, 0,
                     GL_RGBA, GL_FLOAT, nullptr);
        gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, gl::clamp_to_edge);
        gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, gl::clamp_to_edge);

        gl::GenTextures(1, &scene_depth_);
        gl::ActiveTexture(gl::texture0 + 4);
        gl::BindTexture(GL_TEXTURE_2D, scene_depth_);
        gl::TexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(gl::depth_component24), width, height, 0,
                     GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr);
        gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, gl::clamp_to_edge);
        gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, gl::clamp_to_edge);
    }

    gl::ReadBuffer(gl::color_attachment0);
    gl::ActiveTexture(gl::texture0 + 3);
    gl::BindTexture(GL_TEXTURE_2D, scene_copy_);
    gl::CopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, width, height);

    gl::ActiveTexture(gl::texture0 + 4);
    gl::BindTexture(GL_TEXTURE_2D, scene_depth_);
    gl::CopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, width, height);
}

void WaterSimulationTechnique::render(
    const TechniqueContext& frame, const scene::SceneSpine& scene,
    const ResourceSpine& resources) const {
    if (!frame.controls.water_simulation) return;

    gl::Disable(GL_BLEND);
    gl::DepthMask(GL_FALSE);
    gl::Disable(GL_CULL_FACE);

    shader_.bind();
    shader_.set("uViewProjection", frame.view_projection);
    shader_.set("uCameraPosition", frame.camera_position);
    shader_.set("uSunDirection", scene.sun.direction);
    shader_.set("uSunColor", scene.sun.color * frame.controls.sun_intensity);
    shader_.set("uEnvironmentStrength", frame.controls.environment_strength);
    shader_.set("uFogEnabled", frame.controls.fog ? 1 : 0);
    shader_.set("uFogDensity", frame.controls.fog_density);
    shader_.set("uFogHeightFalloff", frame.controls.fog_height_falloff);
    shader_.set("uTime", frame.elapsed_seconds * frame.controls.animation_speed);
    shader_.set("uNormalMap", 0);
    shader_.set("uFoamMap", 1);
    shader_.set("uEnvironmentMap", 2);
    shader_.set("uSceneColor", 3);
    shader_.set("uSceneDepth", 4);
    shader_.set("uViewportSize", math::Vec2{static_cast<float>(frame.width), static_cast<float>(frame.height)});
    shader_.set("uNearPlane", frame.camera_near);
    shader_.set("uFarPlane", frame.camera_far);
    shader_.set("uRefractionStrength", frame.controls.water_refraction_strength);
    resources.texture(normal_map_).bind(0);
    resources.texture(foam_map_).bind(1);
    resources.cubemap(scene.environment).bind(2);
    gl::ActiveTexture(gl::texture0 + 3);
    gl::BindTexture(GL_TEXTURE_2D, scene_copy_);
    gl::ActiveTexture(gl::texture0 + 4);
    gl::BindTexture(GL_TEXTURE_2D, scene_depth_);

    for (const auto& surface : scene.water_surfaces) {
        if (!surface.visible) continue;
        shader_.set("uModel", surface.transform.matrix());
        shader_.set("uShallowColor", surface.shallow_color);
        shader_.set("uDeepColor", surface.deep_color);
        shader_.set("uWaveAmplitude", surface.wave_amplitude * frame.controls.water_wave_strength);
        shader_.set("uWaveSpeed", surface.wave_speed);
        shader_.set("uFlowDirection", surface.flow_direction);
        shader_.set("uSurfaceKind", static_cast<int>(surface.kind));
        shader_.set("uRoughness", surface.roughness);
        shader_.set("uOpacity", surface.opacity);
        shader_.set("uFoamStrength", surface.foam_strength);
        resources.mesh(grid_mesh_).draw();
    }

    gl::Enable(GL_CULL_FACE);
    gl::DepthMask(GL_TRUE);
}

} // namespace epoch::render::techniques
