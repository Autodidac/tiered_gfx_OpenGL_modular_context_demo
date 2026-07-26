#pragma once

#include <string_view>
#include <X11/Xlib.h>
#ifdef CursorShape
#undef CursorShape
#endif

#if defined(EPOCH_NO_MODULES)
#include "epoch/compat/epoch.context.input.hpp"
#else
import epoch.context.input;
#endif

namespace epoch::platform::linuxx {

class X11GlxWindow {
public:
    explicit X11GlxWindow(context::WindowState& state);
    ~X11GlxWindow();

    X11GlxWindow(const X11GlxWindow&) = delete;
    X11GlxWindow& operator=(const X11GlxWindow&) = delete;

    void show() noexcept;
    void poll_events() noexcept;
    void swap() const noexcept;
    void set_vsync(bool enabled) noexcept;
    [[nodiscard]] int swap_interval() const noexcept { return swap_interval_; }
    void set_title(std::string_view title) const noexcept;
    void set_cursor(context::CursorShape shape) noexcept;
    [[nodiscard]] void* native_handle() const noexcept;

private:
    void create_window_and_context();
    void destroy() noexcept;
    void handle_key(const XKeyEvent& event, bool pressed) noexcept;
    void handle_button(const XButtonEvent& event, bool pressed) noexcept;
    void set_pointer_hidden(bool hidden) noexcept;

    context::WindowState& state_;
    Display* display_{};
    ::Window window_{};
    Colormap colormap_{};
    Atom wm_delete_{};
    void* gl_module_{};
    void* context_{};
    Cursor arrow_cursor_{};
    Cursor resize_cursor_{};
    Cursor text_cursor_{};
    Cursor blank_cursor_{};
    int swap_interval_{};
    bool pointer_hidden_{};
};

} // namespace epoch::platform::linuxx
