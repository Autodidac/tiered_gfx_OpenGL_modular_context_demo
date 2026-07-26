module;
#include <compare>
#include <cstdint>

export module render.spine;

export namespace epoch::render {

template <typename Tag>
struct Handle {
    std::uint32_t value{};
    [[nodiscard]] constexpr explicit operator bool() const noexcept { return value != 0; }
    auto operator<=>(const Handle&) const = default;
};

using BufferHandle = Handle<struct BufferTag>;
using MeshHandle = Handle<struct MeshTag>;
using TextureHandle = Handle<struct TextureTag>;
using CubeTextureHandle = Handle<struct CubeTextureTag>;
using SamplerHandle = Handle<struct SamplerTag>;
using ShaderHandle = Handle<struct ShaderTag>;
using PipelineHandle = Handle<struct PipelineTag>;
using MaterialHandle = Handle<struct MaterialTag>;
using RenderTargetHandle = Handle<struct RenderTargetTag>;
using BindingSetHandle = Handle<struct BindingSetTag>;

struct RenderCapabilities {
    std::uint32_t api_major{};
    std::uint32_t api_minor{};
    std::uint32_t max_texture_size{};
    float max_anisotropy{1.0f};
    bool debug_output{};
    bool multiple_render_targets{};
    bool texture_anisotropy{};
    bool geometry_shaders{};
    bool tessellation_shaders{};
    bool indirect_draw{};
    bool compute_shaders{};
    bool bindless_textures{};
};

enum class CapabilityState : std::uint8_t {
    missing,
    partial,
    present,
    deferred
};

struct RendererSpineState {
    CapabilityState resource_model{CapabilityState::partial};
    CapabilityState scene_extraction{CapabilityState::partial};
    CapabilityState pass_scheduler{CapabilityState::partial};
    CapabilityState synchronization{CapabilityState::missing};
    CapabilityState debug_hooks{CapabilityState::partial};
};

} // namespace epoch::render
