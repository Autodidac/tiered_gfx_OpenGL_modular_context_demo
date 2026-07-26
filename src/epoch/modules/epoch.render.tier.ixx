module;
#include <cstdint>
#include <string_view>

export module epoch.render.tier;

export namespace epoch::render {

enum class RendererTier : std::uint8_t {
    tier0_mobile_switch,
    tier1_desktop
};

struct TierProfile {
    RendererTier tier{RendererTier::tier0_mobile_switch};
    std::string_view name{"Tier 0 mobile / Switch / GTX 1660"};
    bool vertex_instancing{true};
    bool vertex_billboards{true};
    bool framebuffer_rtt{true};
    bool one_point_shadow_cubemap{true};
    bool cpu_cloth{true};
    bool vertex_water{true};
    bool parallax_optional{true};
    bool geometry_shaders{};
    bool tessellation{};
    bool indirect_draw{};
    bool screen_space_ao{};
    bool gpu_queries{};
    std::uint8_t maximum_spotlights{1};
    std::uint8_t maximum_point_lights{4};
    std::uint8_t directional_shadow_taps{9};
    bool pcss_soft_shadows{};
};

inline constexpr TierProfile tier0_profile{};
inline constexpr TierProfile tier1_profile{
    .tier = RendererTier::tier1_desktop,
    .name = "Tier 1 desktop optional paths",
    .vertex_instancing = true,
    .vertex_billboards = true,
    .framebuffer_rtt = true,
    .one_point_shadow_cubemap = true,
    .cpu_cloth = true,
    .vertex_water = true,
    .parallax_optional = true,
    .geometry_shaders = false,
    .tessellation = true,
    .indirect_draw = true,
    .screen_space_ao = true,
    .gpu_queries = true,
    .maximum_spotlights = 3,
    .maximum_point_lights = 4,
    .directional_shadow_taps = 24,
    .pcss_soft_shadows = true
};

[[nodiscard]] constexpr const TierProfile& active_profile(bool tier0) noexcept {
    return tier0 ? tier0_profile : tier1_profile;
}

} // namespace epoch::render
