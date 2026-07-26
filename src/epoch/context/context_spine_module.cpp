module;
#include <memory>
#include "epoch/context/detail/context_spine_bridge.hpp"

module epoch.context.spine;

namespace epoch::context {

class ContextSpine::Impl final {
public:
    explicit Impl(NativeApplicationInstance instance)
        : handle{epoch_context_spine_create(instance)} {}

    ~Impl() { epoch_context_spine_destroy(handle); }

    void* handle{};
};

ContextSpine::ContextSpine(NativeApplicationInstance instance)
    : impl_{std::make_unique<Impl>(instance)} {}

ContextSpine::~ContextSpine() = default;
ContextSpine::ContextSpine(ContextSpine&&) noexcept = default;
ContextSpine& ContextSpine::operator=(ContextSpine&&) noexcept = default;

bool ContextSpine::begin_frame(FrameContext& frame) {
    return epoch_context_spine_begin_frame(impl_->handle, &frame);
}

void ContextSpine::end_frame() noexcept { epoch_context_spine_end_frame(impl_->handle); }
void ContextSpine::apply_runtime_controls() { epoch_context_spine_apply_controls(impl_->handle); }

InputState& ContextSpine::input() noexcept {
    return *static_cast<InputState*>(epoch_context_spine_input(impl_->handle));
}

const RuntimeControls& ContextSpine::controls() const noexcept {
    return *static_cast<const RuntimeControls*>(epoch_context_spine_controls_const(impl_->handle));
}

RuntimeControls& ContextSpine::controls() noexcept {
    return *static_cast<RuntimeControls*>(epoch_context_spine_controls(impl_->handle));
}

NativeWindowHandle ContextSpine::native_window() const noexcept {
    return epoch_context_spine_native_window(impl_->handle);
}

void ContextSpine::update_title(float fps) const noexcept {
    epoch_context_spine_update_title(impl_->handle, fps);
}

} // namespace epoch::context
