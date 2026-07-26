#include "epoch/context/detail/context_spine_impl.hpp"
#include "epoch/context/detail/context_spine_bridge.hpp"

extern "C" {

void* epoch_context_spine_create(void* native_instance) {
    return new epoch::context::detail::ContextSpineImpl{native_instance};
}

void epoch_context_spine_destroy(void* handle) noexcept {
    delete static_cast<epoch::context::detail::ContextSpineImpl*>(handle);
}

bool epoch_context_spine_begin_frame(void* handle, void* frame) {
    return static_cast<epoch::context::detail::ContextSpineImpl*>(handle)->begin_frame(
        *static_cast<epoch::context::FrameContext*>(frame));
}

void epoch_context_spine_end_frame(void* handle) noexcept {
    static_cast<epoch::context::detail::ContextSpineImpl*>(handle)->end_frame();
}

void epoch_context_spine_apply_controls(void* handle) {
    static_cast<epoch::context::detail::ContextSpineImpl*>(handle)->apply_runtime_controls();
}

void* epoch_context_spine_input(void* handle) noexcept {
    return &static_cast<epoch::context::detail::ContextSpineImpl*>(handle)->input();
}

const void* epoch_context_spine_controls_const(const void* handle) noexcept {
    return &static_cast<const epoch::context::detail::ContextSpineImpl*>(handle)->controls();
}

void* epoch_context_spine_controls(void* handle) noexcept {
    return &static_cast<epoch::context::detail::ContextSpineImpl*>(handle)->controls();
}

void* epoch_context_spine_native_window(const void* handle) noexcept {
    return static_cast<const epoch::context::detail::ContextSpineImpl*>(handle)->native_window();
}

void epoch_context_spine_update_title(const void* handle, float fps) noexcept {
    static_cast<const epoch::context::detail::ContextSpineImpl*>(handle)->update_title(fps);
}

} // extern "C"
