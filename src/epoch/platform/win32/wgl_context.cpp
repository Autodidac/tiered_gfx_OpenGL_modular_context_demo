#include "epoch/platform/win32/wgl_context.hpp"
#include "epoch/render/gl/gl_api.hpp"
#include <stdexcept>

namespace epoch::platform::win32 {
namespace {
inline constexpr int wgl_draw_to_window_arb = 0x2001;
inline constexpr int wgl_support_opengl_arb = 0x2010;
inline constexpr int wgl_double_buffer_arb = 0x2011;
inline constexpr int wgl_pixel_type_arb = 0x2013;
inline constexpr int wgl_type_rgba_arb = 0x202B;
inline constexpr int wgl_color_bits_arb = 0x2014;
inline constexpr int wgl_depth_bits_arb = 0x2022;
inline constexpr int wgl_stencil_bits_arb = 0x2023;
inline constexpr int wgl_sample_buffers_arb = 0x2041;
inline constexpr int wgl_samples_arb = 0x2042;
inline constexpr int wgl_context_major_version_arb = 0x2091;
inline constexpr int wgl_context_minor_version_arb = 0x2092;
inline constexpr int wgl_context_flags_arb = 0x2094;
inline constexpr int wgl_context_profile_mask_arb = 0x9126;
inline constexpr int wgl_context_debug_bit_arb = 0x0001;
inline constexpr int wgl_context_core_profile_bit_arb = 0x00000001;

using ChoosePixelFormatArbFn = BOOL(WINAPI*)(HDC, const int*, const FLOAT*, UINT, int*, UINT*);
using CreateContextAttribsArbFn = HGLRC(WINAPI*)(HDC, HGLRC, const int*);
using SwapIntervalExtFn = BOOL(WINAPI*)(int);
using GetSwapIntervalExtFn = int(WINAPI*)();
ChoosePixelFormatArbFn choose_pixel_format_arb{};
CreateContextAttribsArbFn create_context_attribs_arb{};
SwapIntervalExtFn swap_interval_ext{};
GetSwapIntervalExtFn get_swap_interval_ext{};

LRESULT CALLBACK dummy_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
    return DefWindowProcW(hwnd, message, wparam, lparam);
}
}

WglContext::WglContext(const Window& window) : window_{window} {
    load_extensions();
    choose_real_pixel_format();
    create_core_context();
    render::gl::load_all();
    swap_interval_ext = reinterpret_cast<SwapIntervalExtFn>(render::gl::proc_address("wglSwapIntervalEXT"));
    get_swap_interval_ext = reinterpret_cast<GetSwapIntervalExtFn>(render::gl::proc_address("wglGetSwapIntervalEXT"));
}

WglContext::~WglContext() {
    if (context_) {
        wglMakeCurrent(nullptr, nullptr);
        wglDeleteContext(context_);
    }
}

void WglContext::swap() const noexcept { SwapBuffers(window_.device_context()); }
void WglContext::set_vsync(bool enabled) const noexcept {
    if (swap_interval_ext) {
        // Reapply interval zero when uncapped. Some Windows drivers preserve a
        // previous interval across context recreation until the first explicit call.
        swap_interval_ext(enabled ? 1 : 0);
    }
}
int WglContext::swap_interval() const noexcept { return get_swap_interval_ext ? get_swap_interval_ext() : -1; }

void WglContext::load_extensions() {
    constexpr wchar_t dummy_class[] = L"EpochWglBootstrap";
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc); wc.style = CS_OWNDC; wc.lpfnWndProc = dummy_proc;
    wc.hInstance = GetModuleHandleW(nullptr); wc.lpszClassName = dummy_class;
    RegisterClassExW(&wc);
    HWND dummy = CreateWindowExW(0, dummy_class, L"", WS_OVERLAPPEDWINDOW, 0, 0, 1, 1, nullptr, nullptr, wc.hInstance, nullptr);
    if (!dummy) throw std::runtime_error("WGL bootstrap window creation failed");
    HDC dc = GetDC(dummy);
    PIXELFORMATDESCRIPTOR pfd{};
    pfd.nSize = sizeof(pfd); pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA; pfd.cColorBits = 32; pfd.cDepthBits = 24; pfd.cStencilBits = 8; pfd.iLayerType = PFD_MAIN_PLANE;
    const int format = ChoosePixelFormat(dc, &pfd);
    if (!format || !SetPixelFormat(dc, format, &pfd)) throw std::runtime_error("WGL bootstrap pixel format failed");
    HGLRC rc = wglCreateContext(dc);
    if (!rc || !wglMakeCurrent(dc, rc)) throw std::runtime_error("WGL bootstrap context failed");
    choose_pixel_format_arb = reinterpret_cast<ChoosePixelFormatArbFn>(render::gl::proc_address("wglChoosePixelFormatARB"));
    create_context_attribs_arb = reinterpret_cast<CreateContextAttribsArbFn>(render::gl::proc_address("wglCreateContextAttribsARB"));
    wglMakeCurrent(nullptr, nullptr); wglDeleteContext(rc); ReleaseDC(dummy, dc); DestroyWindow(dummy); UnregisterClassW(dummy_class, wc.hInstance);
    if (!choose_pixel_format_arb || !create_context_attribs_arb) throw std::runtime_error("Required WGL_ARB extensions are unavailable");
}

void WglContext::choose_real_pixel_format() {
    int format{}; UINT count{};
    const int msaa[] = {
        wgl_draw_to_window_arb, TRUE, wgl_support_opengl_arb, TRUE, wgl_double_buffer_arb, TRUE,
        wgl_pixel_type_arb, wgl_type_rgba_arb, wgl_color_bits_arb, 32,
        wgl_depth_bits_arb, 24, wgl_stencil_bits_arb, 8,
        wgl_sample_buffers_arb, 1, wgl_samples_arb, 4, 0
    };
    const int fallback[] = {
        wgl_draw_to_window_arb, TRUE, wgl_support_opengl_arb, TRUE, wgl_double_buffer_arb, TRUE,
        wgl_pixel_type_arb, wgl_type_rgba_arb, wgl_color_bits_arb, 32,
        wgl_depth_bits_arb, 24, wgl_stencil_bits_arb, 8, 0
    };
    HDC dc = window_.device_context();
    if (!choose_pixel_format_arb(dc, msaa, nullptr, 1, &format, &count) || count == 0) {
        if (!choose_pixel_format_arb(dc, fallback, nullptr, 1, &format, &count) || count == 0)
            throw std::runtime_error("No suitable WGL pixel format found");
    }
    PIXELFORMATDESCRIPTOR pfd{};
    DescribePixelFormat(dc, format, sizeof(pfd), &pfd);
    if (!SetPixelFormat(dc, format, &pfd)) throw std::runtime_error("SetPixelFormat failed");
}

void WglContext::create_core_context() {
    int flags = 0;
#ifndef NDEBUG
    flags |= wgl_context_debug_bit_arb;
#endif
    const int attributes[] = {
        wgl_context_major_version_arb, 4, wgl_context_minor_version_arb, 5,
        wgl_context_profile_mask_arb, wgl_context_core_profile_bit_arb,
        wgl_context_flags_arb, flags, 0
    };
    context_ = create_context_attribs_arb(window_.device_context(), nullptr, attributes);
    if (!context_) throw std::runtime_error("OpenGL 4.5 core context creation failed");
    if (!wglMakeCurrent(window_.device_context(), context_)) throw std::runtime_error("wglMakeCurrent failed");
}

} // namespace epoch::platform::win32
