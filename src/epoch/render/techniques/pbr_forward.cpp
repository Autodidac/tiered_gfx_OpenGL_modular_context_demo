#include "epoch/render/techniques/pbr_forward.hpp"
#include <algorithm>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

namespace epoch::render::techniques {

PbrForwardTechnique::PbrForwardTechnique(const std::filesystem::path& root, ResourceSpine& resources)
    : shader_{root / "pbr/pbr.vert", root / "pbr/pbr.frag"},
      projector_gobo_{resources.load_texture("curated/projector_gobo.png", gl::ColorSpace::srgb)} {}

void PbrForwardTechnique::render(const TechniqueContext& frame, const scene::SceneSpine& scene,
                                 const ResourceSpine& resources, const ShadowMappingTechnique& shadows,
                                 const PointShadowMappingTechnique& point_shadows) const {
    shader_.bind();
    shader_.set("uViewProjection", frame.view_projection);
    shader_.set("uLightViewProjection", frame.light_view_projection);
    shader_.set("uProjectorViewProjection", frame.projector_view_projection);
    shader_.set("uCameraPosition", frame.camera_position);
    shader_.set("uSunDirection", scene.sun.direction);
    shader_.set("uSunRadiance", scene.sun.color);
    shader_.set("uPointLightCount", static_cast<int>(scene.point_light_count));
    shader_.set("uDirectionalEnabled", frame.controls.directional_light ? 1 : 0);
    shader_.set("uPointLightsEnabled", frame.controls.point_lights ? 1 : 0);
    shader_.set("uEnvironmentEnabled", frame.controls.environment_lighting ? 1 : 0);
    shader_.set("uNormalMappingEnabled", frame.controls.normal_mapping ? 1 : 0);
    shader_.set("uShadowsEnabled", frame.controls.shadows ? 1 : 0);
    shader_.set("uPointShadowEnabled", frame.controls.point_shadows ? 1 : 0);
    shader_.set("uPointShadowLightIndex", point_shadows.light_index());
    shader_.set("uPointShadowFarPlane", point_shadows.far_plane(scene));
    shader_.set("uParallaxEnabled", frame.controls.parallax ? 1 : 0);
    shader_.set("uFogEnabled", frame.controls.fog ? 1 : 0);
    shader_.set("uToonEnabled", frame.controls.toon ? 1 : 0);
    shader_.set("uRimEnabled", frame.controls.rim_lighting ? 1 : 0);
    shader_.set("uProjectedTextureEnabled", frame.controls.projected_texture ? 1 : 0);
    shader_.set("uClearcoatEnabled", frame.controls.clearcoat ? 1 : 0);
    shader_.set("uReflectionRefractionEnabled", frame.controls.reflection_refraction ? 1 : 0);
    shader_.set("uGlobalClearcoatStrength", frame.controls.clearcoat_strength);
    shader_.set("uGlobalTransmissionStrength", frame.controls.transmission_strength);
    shader_.set("uProjectorStrength", frame.controls.projector_strength);
    shader_.set("uShadingMode", static_cast<int>(frame.controls.shading_mode));
    shader_.set("uTier0Profile", frame.controls.tier0_mobile_profile ? 1 : 0);
    shader_.set("uSunIntensity", frame.controls.sun_intensity);
    shader_.set("uEnvironmentStrength", frame.controls.environment_strength);
    shader_.set("uGlobalNormalStrength", frame.controls.normal_strength);
    shader_.set("uGlobalParallaxStrength", frame.controls.parallax_strength);
    shader_.set("uFogDensity", frame.controls.fog_density);
    shader_.set("uFogHeightFalloff", frame.controls.fog_height_falloff);
    shader_.set("uEditorDebugOverride", 0);
    shader_.set("uEditorDebugColor", math::Vec4{1.0f, 0.0f, 1.0f, 1.0f});

    const auto upload_spot = [&](const scene::SpotLight& spot, std::string_view suffix) {
        const std::string position_name = "uSpot" + std::string(suffix) + "PositionRange";
        const std::string direction_name = "uSpot" + std::string(suffix) + "DirectionOuter";
        const std::string color_name = "uSpot" + std::string(suffix) + "ColorIntensity";
        const std::string inner_name = "uSpot" + std::string(suffix) + "InnerCosine";
        const std::string enabled_name = "uSpot" + std::string(suffix) + "Enabled";
        const std::string dual_name = "uSpot" + std::string(suffix) + "DualSided";
        shader_.set(position_name.c_str(), math::Vec4{spot.position.x, spot.position.y, spot.position.z, spot.range});
        shader_.set(direction_name.c_str(), math::Vec4{spot.direction.x, spot.direction.y, spot.direction.z, spot.outer_cosine});
        shader_.set(color_name.c_str(), math::Vec4{spot.color.x, spot.color.y, spot.color.z, spot.intensity});
        shader_.set(inner_name.c_str(), spot.inner_cosine);
        shader_.set(enabled_name.c_str(), frame.controls.spot_light && spot.enabled ? 1 : 0);
        shader_.set(dual_name.c_str(), spot.dual_sided ? 1 : 0);
    };
    if (frame.controls.tier0_mobile_profile) {
        auto disabled = scene.spotlight;
        disabled.enabled = false;
        upload_spot(disabled, "");
        upload_spot(disabled, "2");
        upload_spot(scene.projector_spotlight, "3");
    } else {
        upload_spot(scene.spotlight, "");
        upload_spot(scene.secondary_spotlight, "2");
        upload_spot(scene.projector_spotlight, "3");
    }

    for (std::size_t i = 0; i < scene.point_light_count; ++i) {
        char position_name[64]{}; char color_name[64]{};
        std::snprintf(position_name, sizeof(position_name), "uPointPositionRadius[%zu]", i);
        std::snprintf(color_name, sizeof(color_name), "uPointColorIntensity[%zu]", i);
        const auto& light = scene.point_lights[i];
        shader_.set(position_name, math::Vec4{light.position.x, light.position.y, light.position.z, light.radius});
        shader_.set(color_name, math::Vec4{light.color.x, light.color.y, light.color.z, light.intensity});
    }

    shader_.set("uAlbedoMap", 0); shader_.set("uNormalMap", 1); shader_.set("uOrmMap", 2);
    shader_.set("uHeightMap", 3); shader_.set("uEmissiveMap", 4); shader_.set("uShadowMap", 5);
    shader_.set("uEnvironmentMap", 6); shader_.set("uOpacityMap", 7); shader_.set("uProjectedTexture", 8);
    shader_.set("uPointShadowMap", 9);
    shadows.bind_depth(5); resources.cubemap(scene.environment).bind(6); resources.texture(projector_gobo_).bind(8);
    point_shadows.bind_depth(9);

    const auto draw_object = [&](const scene::RenderObject& object, const scene::Material& material,
                                 bool force_double_sided = false) {
        if (force_double_sided || object.double_sided) gl::Disable(GL_CULL_FACE); else gl::Enable(GL_CULL_FACE);
        resources.texture(material.albedo).bind(0); resources.texture(material.normal).bind(1);
        resources.texture(material.orm).bind(2); resources.texture(material.height).bind(3);
        resources.texture(material.emissive).bind(4); resources.texture(material.opacity).bind(7);
        shader_.set("uModel", object.transform.matrix());
        shader_.set("uUvScale", material.uv_scale);
        shader_.set("uBaseColorFactor", material.base_color);
        shader_.set("uEmissiveFactor", material.emissive_factor);
        shader_.set("uMetallicFactor", material.metallic_factor);
        shader_.set("uRoughnessFactor", material.roughness_factor);
        shader_.set("uNormalScale", material.normal_scale);
        shader_.set("uHeightScale", scene::has_feature(material.features, scene::MaterialFeature::parallax_mapping)
            ? material.height_scale : 0.0f);
        shader_.set("uAlphaCutoff", material.alpha_cutoff);
        shader_.set("uClearcoatFactor", material.clearcoat_factor);
        shader_.set("uClearcoatRoughness", material.clearcoat_roughness);
        shader_.set("uTransmissionFactor", material.transmission_factor);
        shader_.set("uIndexOfRefraction", material.index_of_refraction);
        shader_.set("uUnlit", material.unlit ? 1 : 0);
        shader_.set("uMaterialFeatures", static_cast<unsigned>(material.features));
        shader_.set("uReceivesShadow", object.receives_shadow ? 1 : 0);
        resources.mesh(object.mesh).draw();
    };

    // Opaque objects establish depth first. Transmission objects are composited afterwards.
    for (const auto& object : scene.objects) {
        if (!object.visible || object.editor_only
            || object.editor_binding == scene::EditorBindingKind::instanced_prop) continue;
        const auto& material = scene.material(object.material);
        if (scene::has_feature(material.features, scene::MaterialFeature::transmission)) continue;
        draw_object(object, material);
    }

    std::vector<const scene::RenderObject*> transparent_objects;
    transparent_objects.reserve(scene.objects.size());
    for (const auto& object : scene.objects) {
        if (!object.visible || object.editor_only
            || object.editor_binding == scene::EditorBindingKind::instanced_prop) continue;
        const auto& material = scene.material(object.material);
        if (scene::has_feature(material.features, scene::MaterialFeature::transmission))
            transparent_objects.push_back(&object);
    }
    std::ranges::sort(transparent_objects, [&](const auto* lhs, const auto* rhs) {
        const math::Vec3 left_delta = lhs->transform.position - frame.camera_position;
        const math::Vec3 right_delta = rhs->transform.position - frame.camera_position;
        return math::dot(left_delta, left_delta) > math::dot(right_delta, right_delta);
    });

    gl::Enable(GL_BLEND);
    gl::BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    gl::DepthMask(GL_FALSE);
    for (const auto* object : transparent_objects)
        draw_object(*object, scene.material(object->material));
    gl::DepthMask(GL_TRUE);
    gl::Disable(GL_BLEND);

    if (frame.controls.scene_debug_view) {
        const auto debug_color = [](const scene::RenderObject& object, float alpha) noexcept {
            using scene::EditorBindingKind;
            math::Vec4 color{1.0f, 0.10f, 0.75f, alpha}; // hidden regular object
            switch (object.editor_binding) {
            case EditorBindingKind::water_surface:   color = {0.05f, 0.55f, 1.00f, alpha}; break;
            case EditorBindingKind::cloth_object:    color = {0.05f, 1.00f, 0.95f, alpha}; break;
            case EditorBindingKind::rtt_display:     color = {0.70f, 0.25f, 1.00f, alpha}; break;
            case EditorBindingKind::scene_label:     color = {1.00f, 1.00f, 1.00f, alpha}; break;
            case EditorBindingKind::point_light:     color = {1.00f, 0.95f, 0.10f, alpha}; break;
            case EditorBindingKind::spotlight:       color = {1.00f, 0.45f, 0.05f, alpha}; break;
            case EditorBindingKind::terrain_surface:  color = {0.42f, 0.78f, 0.20f, alpha}; break;
            case EditorBindingKind::foliage_region:  color = {0.10f, 1.00f, 0.25f, alpha}; break;
            case EditorBindingKind::particle_emitter:color = {1.00f, 0.15f, 0.05f, alpha}; break;
            case EditorBindingKind::instanced_prop:  color = {0.75f, 0.55f, 0.20f, alpha}; break;
            case EditorBindingKind::none: break;
            }
            return color;
        };
        const auto should_debug = [&](const scene::RenderObject& object) noexcept {
            if (object.editor_deleted || object.camera_attached) return false;
            if (object.editor_only)
                return frame.controls.debug_effect_bounds;
            return !object.visible && frame.controls.debug_hidden_objects;
        };
        const auto draw_debug = [&](bool depth_test, float alpha) {
            if (depth_test) gl::Enable(GL_DEPTH_TEST); else gl::Disable(GL_DEPTH_TEST);
            shader_.set("uEditorDebugOverride", 1);
            for (const auto& object : scene.objects) {
                if (!should_debug(object) || !object.mesh || !object.material
                    || object.material.value > scene.materials.size()) continue;
                shader_.set("uEditorDebugColor", debug_color(object, alpha));
                draw_object(object, scene.material(object.material), true);
            }
        };

        gl::Enable(GL_BLEND);
        gl::BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        gl::DepthMask(GL_FALSE);
        gl::Disable(GL_CULL_FACE);
        gl::PolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        gl::LineWidth(2.0f);
        draw_debug(false, 0.28f);
        draw_debug(true, 0.92f);
        gl::LineWidth(1.0f);
        gl::DepthMask(GL_TRUE);
        gl::Disable(GL_BLEND);
        gl::Enable(GL_DEPTH_TEST);
        shader_.set("uEditorDebugOverride", 0);
        gl::PolygonMode(GL_FRONT_AND_BACK, frame.controls.wireframe ? GL_LINE : GL_FILL);
    }

    gl::Enable(GL_CULL_FACE); gl::CullFace(GL_BACK);
}

} // namespace epoch::render::techniques
