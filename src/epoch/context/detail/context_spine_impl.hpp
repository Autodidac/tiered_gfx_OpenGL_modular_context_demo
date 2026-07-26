#pragma once

#include <chrono>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include "epoch/platform/win32/wgl_context.hpp"
#include "epoch/platform/win32/win32_window.hpp"
#elif defined(__linux__)
#include "epoch/platform/linux/x11_glx_window.hpp"
#endif



#if defined(EPOCH_NO_MODULES)
#include "epoch/compat/epoch.core.time.hpp"
#else
import epoch.core.time;
#endif
#if defined(EPOCH_NO_MODULES)
#include "epoch/compat/epoch.context.input.hpp"
#else
import epoch.context.input;
#endif
#if defined(EPOCH_NO_MODULES)
#include "epoch/compat/epoch.context.frame.hpp"
#else
import epoch.context.frame;
#endif

namespace epoch::context::detail {

class ContextSpineImpl {
public:
    explicit ContextSpineImpl(void* native_instance);
    ContextSpineImpl(const ContextSpineImpl&) = delete;
    ContextSpineImpl& operator=(const ContextSpineImpl&) = delete;

    [[nodiscard]] bool begin_frame(FrameContext& frame);
    void end_frame() noexcept;
    void apply_runtime_controls();

    [[nodiscard]] InputState& input() noexcept { return state_.input; }
    [[nodiscard]] const RuntimeControls& controls() const noexcept { return state_.controls; }
    [[nodiscard]] RuntimeControls& controls() noexcept { return state_.controls; }
    [[nodiscard]] void* native_window() const noexcept;
    void update_title(float fps) const noexcept;

private:
    WindowState state_{};
#if defined(_WIN32)
    platform::win32::Window window_;
    platform::win32::WglContext context_;
#elif defined(__linux__)
    platform::linuxx::X11GlxWindow window_context_;
#else
#error Unsupported desktop platform
#endif
    core::FrameClock clock_{};
    bool applied_vsync_{};
    std::chrono::steady_clock::time_point frame_started_{};
};

} // namespace epoch::context::detail
