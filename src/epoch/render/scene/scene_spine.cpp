module;
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <utility>

module epoch.render.scene;

namespace epoch::render::scene {
namespace {
inline constexpr std::uint32_t all_scene_presets_mask = (1u << 5u) - 1u;

[[nodiscard]] SpotLight& spotlight_at(SceneSpine& scene, std::size_t index) noexcept {
    if (index == 0u) return scene.spotlight;
    if (index == 1u) return scene.secondary_spotlight;
    return scene.projector_spotlight;
}

[[nodiscard]] math::Vec3 rotate_y_direction(math::Vec3 value, float angle) noexcept {
    const float cosine = std::cos(angle);
    const float sine = std::sin(angle);
    return math::normalize(math::Vec3{
        cosine * value.x + sine * value.z,
        value.y,
        -sine * value.x + cosine * value.z
    });
}

[[nodiscard]] math::Vec3 fixture_rotation(math::Vec3 direction, bool lamp) noexcept {
    direction = math::normalize(direction);
    return {
        -std::asin(std::clamp(direction.y, -1.0f, 1.0f)) + (lamp ? math::radians(90.0f) : 0.0f),
        std::atan2(direction.x, direction.z),
        0.0f
    };
}

void enforce_deleted_editor_bindings(SceneSpine& scene) noexcept {
    for (const auto& object : scene.objects) {
        if (!object.editor_deleted) continue;
        switch (object.editor_binding) {
        case EditorBindingKind::water_surface:
            if (object.editor_binding_index < scene.water_surfaces.size())
                scene.water_surfaces[object.editor_binding_index].visible = false;
            break;
        case EditorBindingKind::cloth_object:
            if (object.editor_binding_index < scene.cloth_objects.size())
                scene.cloth_objects[object.editor_binding_index].visible = false;
            break;
        case EditorBindingKind::rtt_display:
            if (object.editor_binding_index < scene.rtt_displays.size())
                scene.rtt_displays[object.editor_binding_index].visible = false;
            break;
        case EditorBindingKind::scene_label:
            if (object.editor_binding_index < scene.labels.size())
                scene.labels[object.editor_binding_index].visible = false;
            break;
        case EditorBindingKind::point_light:
            if (object.editor_binding_index < scene.point_light_count)
                scene.point_lights[object.editor_binding_index].intensity = 0.0f;
            break;
        case EditorBindingKind::spotlight: {
            auto& light = spotlight_at(scene, object.editor_binding_index);
            light.enabled = false;
            light.intensity = 0.0f;
            break;
        }
        case EditorBindingKind::terrain_surface:
            scene.terrain.visible = false;
            break;
        case EditorBindingKind::foliage_region:
            if (object.editor_binding_index < scene.foliage_regions.size())
                scene.foliage_regions[object.editor_binding_index].visible = false;
            break;
        case EditorBindingKind::particle_emitter:
            if (object.editor_binding_index < scene.particle_emitters.size())
                scene.particle_emitters[object.editor_binding_index].visible = false;
            break;
        default:
            break;
        }
    }
}
} // namespace

MaterialHandle SceneSpine::add_material(Material material_value) {
    materials.push_back(std::move(material_value));
    return MaterialHandle{static_cast<std::uint32_t>(materials.size())};
}

Material& SceneSpine::material(MaterialHandle handle) {
    if (!handle || handle.value > materials.size()) throw std::out_of_range("Invalid material handle");
    return materials[handle.value - 1];
}

const Material& SceneSpine::material(MaterialHandle handle) const {
    if (!handle || handle.value > materials.size()) throw std::out_of_range("Invalid material handle");
    return materials[handle.value - 1];
}

void SceneSpine::set_preset(context::ScenePreset preset) {
    if (preset_initialized_ && active_preset_ == preset) return;
    active_preset_ = preset;
    preset_initialized_ = true;
    const std::uint32_t bit = preset_bit(preset);
    for (auto& object : objects) {
        if (object.editor_deleted) object.visible = false;
        else if (object.preset_mask != all_scene_presets_mask)
            object.visible = (object.preset_mask & bit) != 0;
    }
    const auto update_preset_visibility = [bit](auto& value) noexcept {
        if (value.preset_mask != all_scene_presets_mask)
            value.visible = (value.preset_mask & bit) != 0;
    };
    for (auto& water : water_surfaces) update_preset_visibility(water);
    update_preset_visibility(terrain);
    for (auto& foliage : foliage_regions) update_preset_visibility(foliage);
    for (auto& emitter : particle_emitters) update_preset_visibility(emitter);
    for (auto& cloth : cloth_objects) update_preset_visibility(cloth);
    for (auto& label : labels) update_preset_visibility(label);
    for (auto& display : rtt_displays) update_preset_visibility(display);

    // Presets are time-of-day/diagnostic views of one persistent scene, not separate sample rows.
    switch (preset) {
    case context::ScenePreset::material_lab: // Day
        camera.position = {0.0f, 8.2f, 25.5f};
        camera.yaw_degrees = -90.0f;
        camera.pitch_degrees = -13.0f;
        sun.direction = math::normalize(math::Vec3{-0.44f, -1.0f, -0.30f});
        sun.color = {1.08f, 1.01f, 0.91f};
        break;
    case context::ScenePreset::lighting_lab: // Dusk
        camera.position = {-18.0f, 7.2f, 19.0f};
        camera.yaw_degrees = -54.0f;
        camera.pitch_degrees = -11.0f;
        sun.direction = math::normalize(math::Vec3{-0.72f, -0.46f, -0.24f});
        sun.color = {1.12f, 0.70f, 0.42f};
        break;
    case context::ScenePreset::asset_gallery: // Night
        camera.position = {18.0f, 6.5f, 18.5f};
        camera.yaw_degrees = -126.0f;
        camera.pitch_degrees = -10.0f;
        sun.direction = math::normalize(math::Vec3{0.35f, -1.0f, 0.18f});
        sun.color = {0.18f, 0.25f, 0.42f};
        break;
    case context::ScenePreset::diagnostics:
        camera.position = {0.0f, 5.8f, 18.0f};
        camera.yaw_degrees = -90.0f;
        camera.pitch_degrees = -8.0f;
        sun.direction = math::normalize(math::Vec3{-0.35f, -1.0f, -0.22f});
        sun.color = {0.92f, 0.95f, 1.0f};
        break;
    case context::ScenePreset::advanced_lab: // Windy overcast
        camera.position = {0.0f, 12.0f, 30.0f};
        camera.yaw_degrees = -90.0f;
        camera.pitch_degrees = -20.0f;
        sun.direction = math::normalize(math::Vec3{-0.26f, -1.0f, 0.34f});
        sun.color = {0.72f, 0.80f, 0.90f};
        break;
    }
    enforce_deleted_editor_bindings(*this);
}

void SceneSpine::update(float elapsed_seconds, float delta_seconds, const context::RuntimeControls& controls) {
    set_preset(controls.scene_preset);

    // One continuous, accelerated day/night clock drives the same persistent scene.
    // The speed is expressed in full cycles per minute so it remains obvious in a
    // renderer showcase and can be reduced to production values by the engine.
    if (controls.day_night_cycle && controls.animation) {
        constexpr float tau = 6.28318530718f;
        const float cycles_per_second = std::clamp(controls.day_night_speed, 0.01f, 4.0f) / 60.0f;
        const float solar_angle = elapsed_seconds * cycles_per_second * tau + 1.57079632679f;
        const float elevation = std::sin(solar_angle);
        sun.direction = math::normalize(math::Vec3{
            std::cos(solar_angle) * 0.78f,
            -elevation,
            std::sin(solar_angle) * 0.52f
        });

        // The sun is a direct light, not a night-time fill light. Fade direct
        // radiance across the horizon and let the environment/room lights own night.
        const float above_horizon = std::clamp((elevation + 0.035f) / 0.16f, 0.0f, 1.0f);
        const float daylight_mix = std::clamp((elevation - 0.04f) / 0.42f, 0.0f, 1.0f);
        const math::Vec3 day_color{1.02f, 0.97f, 0.88f};
        const math::Vec3 horizon_color{1.10f, 0.46f, 0.20f};
        sun.color = (horizon_color * (1.0f - daylight_mix) + day_color * daylight_mix)
            * above_horizon;

    }

    const math::Vec3 camera_forward = camera.forward();
    const math::Vec3 flat_forward = math::normalize(math::Vec3{camera_forward.x, 0.0f, camera_forward.z});
    const math::Vec3 camera_right = math::normalize(math::cross(flat_forward, math::Vec3{0.0f, 1.0f, 0.0f}));
    constexpr math::Vec3 world_up{0.0f, 1.0f, 0.0f};
    const float player_yaw = math::radians(camera.yaw_degrees + 90.0f);

    const auto update_spotlight_rotation = [&](SpotLight& light) noexcept {
        if (controls.animation)
            light.direction = rotate_y_direction(
                light.authored_direction,
                elapsed_seconds * controls.animation_speed * light.rotation_speed);
        else
            light.direction = light.authored_direction;
    };
    update_spotlight_rotation(spotlight);
    update_spotlight_rotation(secondary_spotlight);
    update_spotlight_rotation(projector_spotlight);

    for (auto& object : objects) {
        if (object.camera_attached) {
            object.transform.position = camera.position
                + camera_right * object.camera_local_offset.x
                + world_up * object.camera_local_offset.y
                + flat_forward * object.camera_local_offset.z;
            object.transform.rotation = object.camera_rotation_offset + math::Vec3{0.0f, player_yaw, 0.0f};
        } else {
            object.transform.position = object.authored_position
                + object.explode_direction * controls.bench_explode;
            if (object.name == "Red spotlight rotating yoke")
                object.transform.rotation = fixture_rotation(spotlight.direction, false);
            else if (object.name == "Red dual-sided spotlight lamp")
                object.transform.rotation = fixture_rotation(spotlight.direction, true);
            else if (object.name == "Blue spotlight rotating yoke")
                object.transform.rotation = fixture_rotation(secondary_spotlight.direction, false);
            else if (object.name == "Blue dual-sided spotlight lamp")
                object.transform.rotation = fixture_rotation(secondary_spotlight.direction, true);
            else if (object.name == "Projector spotlight rotating yoke")
                object.transform.rotation = fixture_rotation(projector_spotlight.direction, false);
            else if (object.name == "Projector cookie spotlight lamp")
                object.transform.rotation = fixture_rotation(projector_spotlight.direction, true);
        }
    }

    if (!controls.animation) {
        enforce_deleted_editor_bindings(*this);
        return;
    }

    const float scaled_delta = delta_seconds * controls.animation_speed;
    for (auto& object : objects) {
        if (object.visible && !object.camera_attached)
            object.transform.rotation += object.angular_velocity * scaled_delta;
    }

    // Local-light intensity is authored/editor-controlled and never rewritten by time of day.
    enforce_deleted_editor_bindings(*this);
}

} // namespace epoch::render::scene
