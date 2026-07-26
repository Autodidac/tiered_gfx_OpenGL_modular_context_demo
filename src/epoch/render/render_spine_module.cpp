module;
#include <filesystem>
#include <memory>
#include <utility>
#include "epoch/render/detail/render_spine_bridge.hpp"

module epoch.render.spine;

namespace epoch::render {

class RenderSpine::Impl final {
public:
    explicit Impl(const std::filesystem::path& asset_root)
        : handle{epoch_render_spine_create(&asset_root)} {}

    ~Impl() { epoch_render_spine_destroy(handle); }

    void* handle{};
};

RenderSpine::RenderSpine(std::filesystem::path asset_root)
    : impl_{std::make_unique<Impl>(asset_root)} {}

RenderSpine::~RenderSpine() = default;
RenderSpine::RenderSpine(RenderSpine&&) noexcept = default;
RenderSpine& RenderSpine::operator=(RenderSpine&&) noexcept = default;

void RenderSpine::update(context::InputState& input,
                         const context::FrameContext& frame,
                         context::RuntimeControls& controls) {
    epoch_render_spine_update(impl_->handle, &input, &frame, &controls);
}

void RenderSpine::render(const context::FrameContext& frame,
                         const context::RuntimeControls& controls) {
    epoch_render_spine_render(impl_->handle, &frame, &controls);
}

const RenderCapabilities& RenderSpine::capabilities() const noexcept {
    return *static_cast<const RenderCapabilities*>(epoch_render_spine_capabilities(impl_->handle));
}

} // namespace epoch::render
