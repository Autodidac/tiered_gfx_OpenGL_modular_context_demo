#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace epoch::context {

enum class ScenePreset : std::uint8_t {
    material_lab,
    lighting_lab,
    asset_gallery,
    diagnostics,
    advanced_lab
};

enum class CursorShape : std::uint8_t {
    arrow,
    horizontal_resize,
    text
};

enum class ShadingMode : std::uint8_t {
    pbr,
    blinn_phong,
    unlit,
    normals,
    uv,
    roughness,
    metallic,
    ambient_occlusion,
    shadow_visibility
};

struct InputState {
    std::array<bool, 256> keys{};
    bool left_mouse_down{};
    bool left_mouse_pressed{};
    bool left_mouse_released{};
    bool right_mouse_down{};
    int mouse_x{};
    int mouse_y{};
    int mouse_dx{};
    int mouse_dy{};
    int mouse_wheel{};
    std::array<char, 32> text_input{};
    std::size_t text_input_count{};
    int last_mouse_x{};
    int last_mouse_y{};
    bool have_last_mouse{};
    CursorShape cursor_shape{CursorShape::arrow};
};

struct RuntimeControls {
    ScenePreset scene_preset{ScenePreset::material_lab};
    ShadingMode shading_mode{ShadingMode::pbr};

    bool wireframe{};
    bool vsync{};
    bool frame_limit{true};
    bool animation{true};
    bool day_night_cycle{true};
    bool show_gui{true};
    bool show_help{true};
    bool show_labels{true};

    bool scene_debug_view{};
    bool debug_hidden_objects{true};
    bool debug_effect_bounds{true};

    bool directional_light{true};
    bool point_lights{true};
    bool spot_light{true};
    bool environment_lighting{true};
    bool normal_mapping{true};
    bool parallax{true};
    bool shadows{true};
    bool point_shadows{true};
    bool bloom{true};
    bool fxaa{true};
    bool fog{true};
    bool toon{true};
    bool rim_lighting{true};
    bool billboards{true};
    bool particles{true};
    bool instancing{true};
    bool render_to_texture{true};
    bool mirror_rtt{true};
    bool indirect_draw{};
    bool projected_texture{true};
    bool clearcoat{true};
    bool reflection_refraction{true};
    bool tessellation{};
    bool pn_triangles{};
    bool cloth_simulation{true};
    bool water_simulation{true};
    bool ssao{};
    bool gpu_queries{};
    bool tier0_mobile_profile{true};

    float target_fps{120.0f};
    float exposure{1.0f};
    float gamma{2.0f};
    float bloom_strength{0.5f};
    float bloom_threshold{0.35f};
    float fog_density{0.001f};
    float fog_height_falloff{0.5f};
    float normal_strength{1.0f};
    float parallax_strength{1.0f};
    float sun_intensity{1.0f};
    float environment_strength{0.03f};
    float animation_speed{1.0f};
    float particle_strength{1.0f};
    float clearcoat_strength{1.0f};
    float transmission_strength{1.0f};
    float projector_strength{2.5f};
    float tessellation_level{8.0f};
    float ssao_strength{1.0f};
    float ssao_radius{0.75f};
    float water_wave_strength{1.0f};
    float water_refraction_strength{1.0f};
    float cloth_wind_strength{0.1f};
    float foliage_density{1.0f};
    float day_night_speed{1.0f};
    float bench_explode{0.5f};
    float gui_scale{1.1f};
};

struct WindowState {
    int width{1600};
    int height{900};
    bool running{true};
    bool resized{true};
    InputState input{};
    RuntimeControls controls{};
};

} // namespace epoch::context
