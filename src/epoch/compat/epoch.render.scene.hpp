#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "epoch/compat/epoch.core.math.hpp"
#include "epoch/compat/epoch.context.input.hpp"
#include "epoch/compat/epoch.render.types.hpp"
namespace epoch::render::scene {

struct Transform {
    math::Vec3 position{};
    math::Vec3 rotation{};
    math::Vec3 scale{1.0f, 1.0f, 1.0f};
    [[nodiscard]] math::Mat4 matrix() const noexcept { return math::transform(position, rotation, scale); }
};

enum class EditorBindingKind : std::uint8_t {
    none,
    water_surface,
    cloth_object,
    rtt_display,
    scene_label,
    point_light,
    spotlight,
    terrain_surface,
    foliage_region,
    particle_emitter,
    instanced_prop
};

struct RenderObject {
    std::string name;
    std::string editor_group;
    EditorBindingKind editor_binding{EditorBindingKind::none};
    std::size_t editor_binding_index{};
    std::size_t editor_duplicate_source{static_cast<std::size_t>(-1)};
    std::array<float, 4> editor_properties{};
    MeshHandle mesh{};
    MaterialHandle material{};
    Transform transform{};
    math::Vec3 authored_position{};
    // Absolute authored dimensions and a separate relative multiplier. The render
    // transform stores their product so existing render paths remain compact.
    math::Vec3 authored_size{1.0f, 1.0f, 1.0f};
    math::Vec3 relative_scale{1.0f, 1.0f, 1.0f};
    math::Vec3 explode_direction{};
    math::Vec3 camera_local_offset{};
    math::Vec3 camera_rotation_offset{};
    bool camera_attached{};
    bool editor_only{};
    bool editor_deleted{};
    bool editor_lock_scale_y{};
    bool casts_shadow{true};
    bool receives_shadow{true};
    bool visible{true};
    bool double_sided{};
    std::uint32_t preset_mask{0xffffffffu};
    float animation_phase{};
    math::Vec3 angular_velocity{};
    math::Vec3 editor_min_scale{0.01f, 0.01f, 0.01f};
    math::Vec3 editor_max_scale{50.0f, 50.0f, 50.0f};
    math::Vec3 editor_bounds_min{-100.0f, -25.0f, -100.0f};
    math::Vec3 editor_bounds_max{100.0f, 75.0f, 100.0f};
    math::Vec3 editor_pick_offset{};
    float editor_pick_radius_scale{1.0f};
};

enum class MaterialFeature : std::uint32_t {
    none              = 0,
    normal_mapping    = 1u << 0,
    parallax_mapping  = 1u << 1,
    environment       = 1u << 2,
    clearcoat         = 1u << 3,
    transmission      = 1u << 4,
    projected_texture = 1u << 5,
    toon              = 1u << 6,
    rim_lighting      = 1u << 7,
    alpha_cutout      = 1u << 8,
};

[[nodiscard]] constexpr MaterialFeature operator|(MaterialFeature lhs, MaterialFeature rhs) noexcept {
    return static_cast<MaterialFeature>(
        static_cast<std::uint32_t>(lhs) | static_cast<std::uint32_t>(rhs));
}

[[nodiscard]] constexpr MaterialFeature operator&(MaterialFeature lhs, MaterialFeature rhs) noexcept {
    return static_cast<MaterialFeature>(
        static_cast<std::uint32_t>(lhs) & static_cast<std::uint32_t>(rhs));
}

constexpr MaterialFeature& operator|=(MaterialFeature& lhs, MaterialFeature rhs) noexcept {
    lhs = lhs | rhs;
    return lhs;
}

[[nodiscard]] constexpr bool has_feature(MaterialFeature value, MaterialFeature feature) noexcept {
    return (value & feature) != MaterialFeature::none;
}

struct Material {
    std::string name;
    TextureHandle albedo{};
    TextureHandle normal{};
    TextureHandle orm{};
    TextureHandle height{};
    TextureHandle emissive{};
    TextureHandle opacity{};
    math::Vec3 base_color{1.0f, 1.0f, 1.0f};
    math::Vec3 emissive_factor{};
    float metallic_factor{1.0f};
    float roughness_factor{1.0f};
    float normal_scale{1.0f};
    float height_scale{};
    float uv_scale{1.0f};
    float alpha_cutoff{};
    float clearcoat_factor{};
    float clearcoat_roughness{0.12f};
    float transmission_factor{};
    float index_of_refraction{1.5f};
    MaterialFeature features{MaterialFeature::environment};
    bool unlit{};

    // Editor-only provenance used to rebuild texture/material overrides from the
    // persistent scene configuration without serializing transient GL handles.
    bool editor_unique{};
    std::string editor_base_material_name{};
    std::string editor_albedo_asset{};
    std::string editor_normal_asset{};
    std::string editor_orm_asset{};
};

struct DirectionalLight {
    math::Vec3 direction{-0.45f, -1.0f, -0.25f};
    math::Vec3 color{4.2f, 3.9f, 3.45f};
};

struct PointLight {
    math::Vec3 position{};
    float radius{8.0f};
    math::Vec3 color{1.0f, 1.0f, 1.0f};
    float intensity{18.0f};
};

struct SpotLight {
    math::Vec3 position{};
    float range{12.0f};
    math::Vec3 direction{0.0f, -1.0f, 0.0f};
    float outer_cosine{0.82f};
    math::Vec3 authored_direction{0.0f, -1.0f, 0.0f};
    float rotation_speed{};
    math::Vec3 color{1.0f, 1.0f, 1.0f};
    float intensity{30.0f};
    float inner_cosine{0.9f};
    bool enabled{true};
    bool dual_sided{};
};

inline constexpr std::size_t max_point_lights = 4;

class Camera {
public:
    math::Vec3 position{0.0f, 3.2f, 10.5f};
    float yaw_degrees{-90.0f};
    float pitch_degrees{-10.0f};
    float speed{6.0f};
    float fov_degrees{60.0f};
    float near_plane{0.08f};
    float far_plane{180.0f};

    [[nodiscard]] math::Vec3 forward() const noexcept;
    [[nodiscard]] math::Mat4 view() const noexcept;
    [[nodiscard]] math::Mat4 projection(float aspect) const noexcept;
    void update(context::InputState& input, float dt);
};

enum class ClothPinMode : std::uint8_t {
    top_edge,
    top_corners,
    left_edge
};

struct ClothObject {
    std::string name;
    math::Vec3 origin{};
    math::Vec3 right_axis{1.0f, 0.0f, 0.0f};
    math::Vec3 down_axis{0.0f, -1.0f, 0.0f};
    math::Vec3 tint{0.18f, 0.42f, 0.82f};
    TextureHandle albedo{};
    TextureHandle normal{};
    TextureHandle orm{};
    math::Vec2 uv_scale{1.0f, 1.0f};
    float width{3.2f};
    float height{2.0f};
    float wind_response{1.0f};
    float initial_billow{0.10f};
    float gravity_scale{1.0f};
    std::uint32_t columns{24};
    std::uint32_t rows{16};
    ClothPinMode pin_mode{ClothPinMode::top_edge};
    std::uint32_t preset_mask{0xffffffffu};
    bool visible{true};
};

enum class WaterSurfaceKind : std::uint8_t {
    pond,
    stream,
    fountain,
    pool
};

struct WaterSurface {
    std::string name;
    std::string editor_group;
    Transform transform{};
    math::Vec3 shallow_color{0.04f, 0.34f, 0.42f};
    math::Vec3 deep_color{0.01f, 0.07f, 0.15f};
    math::Vec2 flow_direction{0.0f, 1.0f};
    float wave_amplitude{0.12f};
    float wave_speed{1.0f};
    float roughness{0.08f};
    float opacity{0.82f};
    float foam_strength{0.25f};
    WaterSurfaceKind kind{WaterSurfaceKind::pond};
    std::uint32_t preset_mask{0xffffffffu};
    bool visible{true};
};

enum class RttFeed : std::uint8_t {
    security_camera,
    overhead_camera,
    ceiling_camera,
    planar_mirror
};

struct RttDisplay {
    std::string name;
    std::string editor_group;
    Transform transform{};
    RttFeed feed{RttFeed::security_camera};
    std::uint32_t preset_mask{0xffffffffu};
    bool visible{true};
};

struct RttCamera {
    math::Vec3 position{};
    math::Vec3 target{};
    math::Vec3 up{0.0f, 1.0f, 0.0f};
    float vertical_fov_degrees{58.0f};
};

struct MirrorPlane {
    math::Vec3 center{};
    math::Vec3 normal{0.0f, 0.0f, 1.0f};
    math::Vec3 up{0.0f, 1.0f, 0.0f};
};

struct EditorModelOption {
    std::string name;
    MeshHandle mesh{};
};

inline constexpr std::size_t foliage_mask_resolution = 64u;
inline constexpr std::size_t foliage_mask_texel_count = foliage_mask_resolution * foliage_mask_resolution;

[[nodiscard]] inline std::array<std::uint8_t, foliage_mask_texel_count> full_foliage_mask() noexcept {
    std::array<std::uint8_t, foliage_mask_texel_count> result{};
    result.fill(255u);
    return result;
}

struct TerrainSurface {
    std::string name{"Main terrain"};
    std::string editor_group{"Terrain"};
    // The terrain patch spans local -1..1 in X/Z. Scale therefore represents
    // half-extents in world space, matching the renderer's existing 64 m field.
    Transform transform{{0.0f, -0.82f, -3.0f}, {}, {32.0f, 1.0f, 32.0f}};
    float height_scale{3.6f};
    std::uint32_t preset_mask{0xffffffffu};
    bool visible{true};
};

struct FoliageRegion {
    std::string name;
    std::string editor_group;
    Transform transform{{0.0f, -0.56f, -2.0f}, {}, {29.0f, 1.0f, 26.0f}};
    math::Vec2 blade_scale{1.0f, 1.0f};
    float density{1.0f};
    float sway{1.0f};
    std::uint32_t preset_mask{0xffffffffu};
    bool visible{true};
    // User paint mask: 255 permits grass, 0 removes it. Automatic solid-object
    // exclusion is applied after this mask, so painting cannot place grass under
    // buildings or props; it may only creep beneath the shrunken footprint edge.
    std::array<std::uint8_t, foliage_mask_texel_count> placement_mask{full_foliage_mask()};
    std::uint64_t placement_mask_revision{};
};

enum class ParticleEmitterKind : std::uint8_t {
    fire,
    motes,
    mist
};

struct ParticleEmitter {
    std::string name;
    std::string editor_group;
    Transform transform{};
    float intensity{1.0f};
    float size_scale{1.0f};
    ParticleEmitterKind kind{ParticleEmitterKind::fire};
    std::uint32_t preset_mask{0xffffffffu};
    bool visible{true};
};

struct SceneLabel {
    std::string text;
    math::Vec3 world_position{};
    math::Vec4 color{0.82f, 0.90f, 1.0f, 1.0f};
    float max_distance{32.0f};
    std::uint32_t preset_mask{0xffffffffu};
    bool visible{true};
};

class SceneSpine {
public:
    Camera camera{};
    DirectionalLight sun{};
    SpotLight spotlight{};
    SpotLight secondary_spotlight{};
    SpotLight projector_spotlight{};
    std::array<PointLight, max_point_lights> point_lights{};
    std::size_t point_light_count{};
    CubeTextureHandle environment{};
    std::vector<Material> materials;
    std::vector<RenderObject> objects;
    std::vector<EditorModelOption> editor_models;
    std::vector<WaterSurface> water_surfaces;
    TerrainSurface terrain{};
    std::vector<FoliageRegion> foliage_regions;
    std::vector<ParticleEmitter> particle_emitters;
    std::vector<ClothObject> cloth_objects;
    std::vector<SceneLabel> labels;
    std::vector<RttDisplay> rtt_displays;
    RttCamera security_camera{};
    RttCamera overhead_camera{};
    RttCamera ceiling_camera{};
    MirrorPlane mirror_plane{};

    [[nodiscard]] MaterialHandle add_material(Material material);
    [[nodiscard]] Material& material(MaterialHandle handle);
    [[nodiscard]] const Material& material(MaterialHandle handle) const;
    void set_preset(context::ScenePreset preset);
    void update(float elapsed_seconds, float delta_seconds, const context::RuntimeControls& controls);
    [[nodiscard]] static constexpr std::uint32_t preset_bit(context::ScenePreset preset) noexcept {
        return 1u << static_cast<std::uint32_t>(preset);
    }

private:
    context::ScenePreset active_preset_{context::ScenePreset::material_lab};
    bool preset_initialized_{};
};

} // namespace epoch::render::scene
