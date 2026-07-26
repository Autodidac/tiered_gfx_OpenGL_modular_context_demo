module;
#include <cstddef>
#include <span>
#include <string_view>

export module epoch.render.techniques.catalog;

export namespace epoch::render::techniques {

enum class TechniqueGroup {
    foundation,
    materials,
    lighting,
    geometry,
    simulation,
    post_process,
    diagnostics
};

enum class TechniqueStatus {
    implemented,
    planned_engine_contract,
    capability_gated
};

struct TechniqueCatalogEntry {
    std::string_view id;
    std::string_view display_name;
    TechniqueGroup group{};
    TechniqueStatus status{};
    std::string_view owner;
    std::string_view scene_use;
};

[[nodiscard]] std::span<const TechniqueCatalogEntry> technique_catalog() noexcept;
[[nodiscard]] std::size_t implemented_technique_count() noexcept;

} // namespace epoch::render::techniques
