#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <string_view>

#if defined(EPOCH_NO_MODULES)
#include "epoch/compat/epoch.context.input.hpp"
#else
import epoch.context.input;
#endif

namespace epoch::platform::win32 {

class Window {
public:
    Window(HINSTANCE instance, context::WindowState& state);
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;
    ~Window();

    [[nodiscard]] HWND native_handle() const noexcept { return hwnd_; }
    [[nodiscard]] HDC device_context() const noexcept { return dc_; }
    void show(int command = SW_SHOWDEFAULT) const noexcept;
    void poll_events() noexcept;
    void set_title(std::string_view title) const noexcept;
    void set_cursor(context::CursorShape shape) const noexcept;

private:
    static LRESULT CALLBACK window_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
    void register_class();

    static constexpr wchar_t class_name_[] = L"EpochIntegratedSceneWindow";
    HINSTANCE instance_{};
    context::WindowState& state_;
    HWND hwnd_{};
    HDC dc_{};
};

} // namespace epoch::platform::win32
