#include "epoch/platform/win32/win32_window.hpp"
#include "app/resource.h"
#include <windowsx.h>
#include <algorithm>
#include <stdexcept>
#include <string>

namespace epoch::platform::win32 {
namespace {
[[nodiscard]] context::WindowState* state_from(HWND hwnd) noexcept {
    return reinterpret_cast<context::WindowState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
}
}

Window::Window(HINSTANCE instance, context::WindowState& state) : instance_{instance}, state_{state} {
    register_class();
    RECT rect{0, 0, state_.width, state_.height};
    AdjustWindowRectEx(&rect, WS_OVERLAPPEDWINDOW, FALSE, 0);
    hwnd_ = CreateWindowExW(0, class_name_, L"Integrated OpenGL Scene",
                            WS_OVERLAPPEDWINDOW,
                            CW_USEDEFAULT, CW_USEDEFAULT,
                            rect.right - rect.left, rect.bottom - rect.top,
                            nullptr, nullptr, instance_, &state_);
    if (!hwnd_) throw std::runtime_error("Win32 window creation failed");
    dc_ = GetDC(hwnd_);
    if (!dc_) throw std::runtime_error("GetDC failed");
}

Window::~Window() {
    if (dc_ && hwnd_) ReleaseDC(hwnd_, dc_);
    if (hwnd_) DestroyWindow(hwnd_);
    UnregisterClassW(class_name_, instance_);
}

void Window::show(int command) const noexcept {
    ShowWindow(hwnd_, command);
    UpdateWindow(hwnd_);
}

void Window::poll_events() noexcept {
    MSG message{};
    while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
        if (message.message == WM_QUIT) state_.running = false;
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
}

void Window::set_title(std::string_view title) const noexcept {
    std::wstring wide(title.begin(), title.end());
    SetWindowTextW(hwnd_, wide.c_str());
}

void Window::set_cursor(context::CursorShape shape) const noexcept {
    const wchar_t* identifier = IDC_ARROW;
    switch (shape) {
    case context::CursorShape::horizontal_resize: identifier = IDC_SIZEWE; break;
    case context::CursorShape::text: identifier = IDC_IBEAM; break;
    case context::CursorShape::arrow: break;
    }
    SetCursor(LoadCursorW(nullptr, identifier));
}

void Window::register_class() {
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = window_proc;
    wc.hInstance = instance_;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hIcon = LoadIconW(instance_, MAKEINTRESOURCEW(IDI_APPLICATION_ICON));
    if (!wc.hIcon) wc.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    wc.hIconSm = static_cast<HICON>(LoadImageW(instance_, MAKEINTRESOURCEW(IDI_APPLICATION_ICON),
                                               IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR));
    if (!wc.hIconSm) wc.hIconSm = wc.hIcon;
    wc.lpszClassName = class_name_;
    if (!RegisterClassExW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
        throw std::runtime_error("RegisterClassExW failed");
}

LRESULT CALLBACK Window::window_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
    auto* state = state_from(hwnd);
    switch (message) {
    case WM_NCCREATE: {
        auto* create = reinterpret_cast<CREATESTRUCTW*>(lparam);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(create->lpCreateParams));
        return DefWindowProcW(hwnd, message, wparam, lparam);
    }
    case WM_CLOSE:
        if (state) state->running = false;
        return 0;
    case WM_DESTROY:
        if (state) state->running = false;
        PostQuitMessage(0);
        return 0;
    case WM_SIZE:
        if (state) {
            state->width = std::max(1, static_cast<int>(LOWORD(lparam)));
            state->height = std::max(1, static_cast<int>(HIWORD(lparam)));
            state->resized = true;
        }
        return 0;
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
        if (state && wparam < state->input.keys.size()) {
            state->input.keys[wparam] = true;
            if (wparam == VK_ESCAPE) state->running = false;
            const bool first_press = (lparam & (1LL << 30)) == 0;
            if (first_press) {
                if (wparam == VK_F1) state->controls.wireframe = !state->controls.wireframe;
                if (wparam == VK_F2) state->controls.vsync = !state->controls.vsync;
                if (wparam == VK_F3) state->controls.show_gui = !state->controls.show_gui;
                if (wparam == VK_F4) state->controls.bloom = !state->controls.bloom;
                if (wparam == VK_F5) state->controls.shadows = !state->controls.shadows;
                if (wparam == VK_F6) state->controls.animation = !state->controls.animation;
                if (wparam == VK_F7) state->controls.scene_debug_view = !state->controls.scene_debug_view;
                if (wparam == VK_F10) state->controls.show_help = !state->controls.show_help;
            }
        }
        return 0;
    case WM_CHAR:
        if (state && state->input.text_input_count < state->input.text_input.size()) {
            const wchar_t character = static_cast<wchar_t>(wparam);
            if (character == L'\b' || character == L'\r'
                || (character >= 32 && character <= 126)) {
                state->input.text_input[state->input.text_input_count++] = static_cast<char>(character);
            }
        }
        return 0;
    case WM_KEYUP:
    case WM_SYSKEYUP:
        if (state && wparam < state->input.keys.size()) state->input.keys[wparam] = false;
        return 0;
    case WM_LBUTTONDOWN:
        if (state) { state->input.left_mouse_down = true; state->input.left_mouse_pressed = true; SetCapture(hwnd); }
        return 0;
    case WM_LBUTTONUP:
        if (state) { state->input.left_mouse_down = false; state->input.left_mouse_released = true; ReleaseCapture(); }
        return 0;
    case WM_MOUSEWHEEL:
        if (state) state->input.mouse_wheel += GET_WHEEL_DELTA_WPARAM(wparam) / WHEEL_DELTA;
        return 0;
    case WM_RBUTTONDOWN:
        if (state) {
            state->input.right_mouse_down = true;
            state->input.have_last_mouse = false;
            SetCapture(hwnd);
            ShowCursor(FALSE);
        }
        return 0;
    case WM_RBUTTONUP:
        if (state) {
            state->input.right_mouse_down = false;
            state->input.have_last_mouse = false;
            ReleaseCapture();
            ShowCursor(TRUE);
        }
        return 0;
    case WM_MOUSEMOVE:
        if (state) {
            const int x = GET_X_LPARAM(lparam);
            const int y = GET_Y_LPARAM(lparam);
            state->input.mouse_x = x;
            state->input.mouse_y = y;
            if (state->input.right_mouse_down) {
            if (state->input.have_last_mouse) {
                state->input.mouse_dx += x - state->input.last_mouse_x;
                state->input.mouse_dy += y - state->input.last_mouse_y;
            }
            state->input.last_mouse_x = x;
            state->input.last_mouse_y = y;
            state->input.have_last_mouse = true;
            }
        }
        return 0;
    default:
        return DefWindowProcW(hwnd, message, wparam, lparam);
    }
}

} // namespace epoch::platform::win32
