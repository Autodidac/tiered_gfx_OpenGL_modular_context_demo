module;
#include <compare>
#include <cstdint>

export module epoch.render.types;

export namespace epoch::render {

template <typename Tag>
struct Handle {
    std::uint32_t value{};
    [[nodiscard]] constexpr explicit operator bool() const noexcept { return value != 0; }
    auto operator<=>(const Handle&) const = default;
};

using MeshHandle = Handle<struct MeshTag>;
using TextureHandle = Handle<struct TextureTag>;
using CubeTextureHandle = Handle<struct CubeTextureTag>;
using MaterialHandle = Handle<struct MaterialTag>;

struct RenderCapabilities {
    int gl_major{};
    int gl_minor{};
    float max_anisotropy{1.0f};
    int max_texture_size{};
    bool debug_output{};
    bool texture_anisotropy{};
};

} // namespace epoch::render
