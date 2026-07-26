#include "epoch/context/detail/context_spine_impl.hpp"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <string>
#include <thread>

namespace epoch::context::detail {

ContextSpineImpl::ContextSpineImpl(void* native_instance)
#if defined(_WIN32)
    : window_{static_cast<HINSTANCE>(native_instance), state_}, context_{window_}
#elif defined(__linux__)
    : window_context_{state_}
#endif
{
    (void)native_instance;
#if defined(_WIN32)
    context_.set_vsync(false);
    window_.show();
#elif defined(__linux__)
    window_context_.set_vsync(false);
    window_context_.show();
#endif
    applied_vsync_ = true;
}

bool ContextSpineImpl::begin_frame(FrameContext& frame) {
    state_.input.cursor_shape = context::CursorShape::arrow;
    state_.input.left_mouse_pressed = false;
    state_.input.left_mouse_released = false;
    state_.input.mouse_wheel = 0;
    state_.input.text_input_count = 0;
#if defined(_WIN32)
    window_.poll_events();
#elif defined(__linux__)
    window_context_.poll_events();
#endif
    if (!state_.running) return false;
    frame_started_ = std::chrono::steady_clock::now();
    frame.delta_seconds = clock_.tick();
    frame.elapsed_seconds = clock_.elapsed_seconds();
    frame.framebuffer_width = state_.width;
    frame.framebuffer_height = state_.height;
    frame.resized = state_.resized;
    state_.resized = false;
    return true;
}

void ContextSpineImpl::end_frame() noexcept {
#if defined(_WIN32)
    if (!state_.input.right_mouse_down) window_.set_cursor(state_.input.cursor_shape);
    context_.swap();
#elif defined(__linux__)
    if (!state_.input.right_mouse_down) window_context_.set_cursor(state_.input.cursor_shape);
    window_context_.swap();
#endif

    const auto& controls = state_.controls;
    if (!controls.frame_limit || controls.vsync) return;
    const float target_fps = std::clamp(controls.target_fps, 30.0f, 500.0f);
    const auto frame_duration = std::chrono::duration<double>{1.0 / static_cast<double>(target_fps)};
    const auto deadline = frame_started_ + std::chrono::duration_cast<std::chrono::steady_clock::duration>(frame_duration);
    auto now = std::chrono::steady_clock::now();
    constexpr auto spin_margin = std::chrono::microseconds{350};
    if (now + spin_margin < deadline) std::this_thread::sleep_until(deadline - spin_margin);
    while (std::chrono::steady_clock::now() < deadline) std::this_thread::yield();
}

void ContextSpineImpl::apply_runtime_controls() {
    if (applied_vsync_ == state_.controls.vsync) return;
#if defined(_WIN32)
    context_.set_vsync(state_.controls.vsync);
#elif defined(__linux__)
    window_context_.set_vsync(state_.controls.vsync);
#endif
    applied_vsync_ = state_.controls.vsync;
}

void* ContextSpineImpl::native_window() const noexcept {
#if defined(_WIN32)
    return static_cast<void*>(window_.native_handle());
#elif defined(__linux__)
    return window_context_.native_handle();
#endif
}

void ContextSpineImpl::update_title(float fps) const noexcept {
    const auto& c = state_.controls;
    char title[512]{};
    std::snprintf(title, sizeof(title),
        "OpenGL Scene | %.0f FPS | Cap:%s %.0f | Swap:%d | F1 Wire:%s | F2 VSync:%s | F3 GUI:%s | F4 Bloom:%s | F5 Shadows:%s | F6 Animate:%s | F7 Debug:%s | RMB Look",
        fps,
        c.frame_limit ? "On" : "Off",
        c.target_fps,
#if defined(_WIN32)
        context_.swap_interval(),
#elif defined(__linux__)
        window_context_.swap_interval(),
#endif
        c.wireframe ? "On" : "Off",
        c.vsync ? "On" : "Off",
        c.show_gui ? "On" : "Off",
        c.bloom ? "On" : "Off",
        c.shadows ? "On" : "Off",
        c.animation ? "On" : "Off",
        c.scene_debug_view ? "On" : "Off");
#if defined(_WIN32)
    window_.set_title(title);
#elif defined(__linux__)
    window_context_.set_title(title);
#endif
}

} // namespace epoch::context::detail
