module;
#include <array>
#include <cstdint>

export module context.spine;

export namespace epoch::context {

enum class ScenePreset : std::uint8_t {
    integrated_day,
    integrated_dusk,
    integrated_night,
    diagnostics,
    windy_overcast
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

struct FrameContext {
    float delta_seconds{};
    float elapsed_seconds{};
    std::uint32_t framebuffer_width{1};
    std::uint32_t framebuffer_height{1};
    bool resized{};
};

struct InputSnapshot {
    std::array<bool, 256> keys{};
    std::int32_t mouse_x{};
    std::int32_t mouse_y{};
    std::int32_t mouse_delta_x{};
    std::int32_t mouse_delta_y{};
    bool primary_down{};
    bool primary_pressed{};
    bool primary_released{};
    bool look_down{};
};

struct RuntimeControls {
    ScenePreset scene_preset{ScenePreset::integrated_day};
    ShadingMode shading_mode{ShadingMode::pbr};

    bool wireframe{};
    bool vsync{};
    bool frame_limit{};
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

    // Stable Tier-0 startup defaults. Advanced desktop paths remain opt-in.
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
    float gui_scale{1.1f};
};

struct ContextFrame {
    FrameContext timing{};
    InputSnapshot input{};
    RuntimeControls controls{};
};

} // namespace epoch::context
