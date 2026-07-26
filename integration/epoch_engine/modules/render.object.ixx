module;
#include <cstdint>

export module render.object;

import render.spine;

export namespace epoch::render {

struct Float2 { float x{}, y{}; };
struct Float3 { float x{}, y{}, z{}; };

struct TransformProxy {
    Float3 position{};
    Float3 rotation_radians{};
    Float3 scale{1.0f, 1.0f, 1.0f};
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
    alpha_cutout      = 1u << 8
};

[[nodiscard]] constexpr MaterialFeature operator|(MaterialFeature lhs, MaterialFeature rhs) noexcept {
    return static_cast<MaterialFeature>(
        static_cast<std::uint32_t>(lhs) | static_cast<std::uint32_t>(rhs));
}

struct MaterialProxy {
    TextureHandle albedo{};
    TextureHandle normal{};
    TextureHandle orm{};
    TextureHandle height{};
    TextureHandle emissive{};
    TextureHandle opacity{};
    Float3 base_color{1.0f, 1.0f, 1.0f};
    Float3 emissive_factor{};
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
};

struct RenderObjectProxy {
    std::uint64_t stable_entity_id{};
    MeshHandle mesh{};
    MaterialHandle material{};
    TransformProxy transform{};
    bool casts_shadow{true};
    bool receives_shadow{true};
    bool double_sided{};
    bool visible{true};
};

struct WaterSurfaceProxy {
    std::uint64_t stable_entity_id{};
    TransformProxy transform{};
    Float3 shallow_color{0.04f, 0.28f, 0.36f};
    Float3 deep_color{0.01f, 0.05f, 0.12f};
    Float2 flow_direction{0.0f, 1.0f};
    float wave_amplitude{0.03f};
    float wave_speed{1.0f};
    float roughness{0.12f};
    float opacity{0.82f};
    float foam_strength{0.04f};
    bool visible{true};
};

enum class ClothPinMode : std::uint8_t {
    top_edge,
    top_corners,
    left_edge
};

struct ClothObjectProxy {
    std::uint64_t stable_entity_id{};
    Float3 origin{};
    Float3 right_axis{1.0f, 0.0f, 0.0f};
    Float3 down_axis{0.0f, -1.0f, 0.0f};
    Float3 tint{1.0f, 1.0f, 1.0f};
    TextureHandle albedo{};
    TextureHandle normal{};
    TextureHandle orm{};
    float width{2.0f};
    float height{1.0f};
    float wind_response{1.0f};
    std::uint32_t columns{24};
    std::uint32_t rows{14};
    ClothPinMode pin_mode{ClothPinMode::top_edge};
    bool visible{true};
};

enum class RttFeed : std::uint8_t {
    security_camera,
    overhead_camera,
    planar_mirror
};

struct RttDisplayProxy {
    std::uint64_t stable_entity_id{};
    TransformProxy transform{};
    RttFeed feed{RttFeed::security_camera};
    bool visible{true};
};

} // namespace epoch::render
