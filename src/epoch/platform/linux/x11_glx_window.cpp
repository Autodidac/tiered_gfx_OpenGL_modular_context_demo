#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <dlfcn.h>
#include <stdexcept>
#include <string>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/cursorfont.h>
#ifdef CursorShape
#undef CursorShape
#endif

#include "epoch/render/gl/gl_api.hpp"
#include "epoch/platform/linux/x11_glx_window.hpp"

namespace epoch::platform::linuxx {
namespace {
using GLXDrawable = XID;
using GLXContext = void*;
using GLXFBConfig = void*;

inline constexpr int glx_x_renderable = 0x8012;
inline constexpr int glx_drawable_type = 0x8010;
inline constexpr int glx_window_bit = 0x00000001;
inline constexpr int glx_render_type = 0x8011;
inline constexpr int glx_rgba_bit = 0x00000001;
inline constexpr int glx_x_visual_type = 0x22;
inline constexpr int glx_true_color = 0x8002;
inline constexpr int glx_red_size = 8;
inline constexpr int glx_green_size = 9;
inline constexpr int glx_blue_size = 10;
inline constexpr int glx_alpha_size = 11;
inline constexpr int glx_depth_size = 12;
inline constexpr int glx_stencil_size = 13;
inline constexpr int glx_doublebuffer = 5;
inline constexpr int glx_sample_buffers = 100000;
inline constexpr int glx_samples = 100001;
inline constexpr int glx_rgba_type = 0x8014;
inline constexpr int glx_context_major_version_arb = 0x2091;
inline constexpr int glx_context_minor_version_arb = 0x2092;
inline constexpr int glx_context_flags_arb = 0x2094;
inline constexpr int glx_context_profile_mask_arb = 0x9126;
inline constexpr int glx_context_debug_bit_arb = 0x0001;
inline constexpr int glx_context_core_profile_bit_arb = 0x00000001;

using ChooseFBConfigFn = GLXFBConfig*(*)(Display*, int, const int*, int*);
using GetVisualFromFBConfigFn = XVisualInfo*(*)(Display*, GLXFBConfig);
using CreateNewContextFn = GLXContext(*)(Display*, GLXFBConfig, int, GLXContext, Bool);
using CreateContextAttribsFn = GLXContext(*)(Display*, GLXFBConfig, GLXContext, Bool, const int*);
using MakeCurrentFn = Bool(*)(Display*, GLXDrawable, GLXContext);
using SwapBuffersFn = void(*)(Display*, GLXDrawable);
using DestroyContextFn = void(*)(Display*, GLXContext);
using SwapIntervalExtFn = void(*)(Display*, GLXDrawable, int);
using SwapIntervalMesaFn = int(*)(unsigned);
using SwapIntervalSgiFn = int(*)(int);

ChooseFBConfigFn choose_fb_config{};
GetVisualFromFBConfigFn get_visual_from_fb_config{};
CreateNewContextFn create_new_context{};
CreateContextAttribsFn create_context_attribs{};
MakeCurrentFn make_current{};
SwapBuffersFn swap_buffers{};
DestroyContextFn destroy_context{};
SwapIntervalExtFn swap_interval_ext{};
SwapIntervalMesaFn swap_interval_mesa{};
SwapIntervalSgiFn swap_interval_sgi{};

void* symbol(void* module, const char* name) {
    void* result = module ? dlsym(module, name) : nullptr;
    if (!result) throw std::runtime_error(std::string{"Missing GLX symbol: "} + name);
    return result;
}

std::size_t portable_key(KeySym key) noexcept {
    if (key >= XK_a && key <= XK_z) return static_cast<std::size_t>('A' + (key - XK_a));
    if (key >= XK_A && key <= XK_Z) return static_cast<std::size_t>(key);
    if (key >= XK_0 && key <= XK_9) return static_cast<std::size_t>(key);
    switch (key) {
    case XK_Shift_L: case XK_Shift_R: return 16u;
    case XK_Control_L: case XK_Control_R: return 17u;
    case XK_Alt_L: case XK_Alt_R: return 18u;
    case XK_Escape: return 27u;
    case XK_F1: return 0x70u;
    case XK_F2: return 0x71u;
    case XK_F3: return 0x72u;
    case XK_F4: return 0x73u;
    case XK_F5: return 0x74u;
    case XK_F6: return 0x75u;
    case XK_F7: return 0x76u;
    case XK_F8: return 0x77u;
    case XK_F9: return 0x78u;
    case XK_F10: return 0x79u;
    default: return 255u;
    }
}

Cursor make_blank_cursor(Display* display, ::Window window) {
    static char empty_data[1]{};
    Pixmap bitmap = XCreateBitmapFromData(display, window, empty_data, 1, 1);
    XColor black{};
    Cursor cursor = XCreatePixmapCursor(display, bitmap, bitmap, &black, &black, 0, 0);
    XFreePixmap(display, bitmap);
    return cursor;
}
}

X11GlxWindow::X11GlxWindow(context::WindowState& state) : state_{state} {
    create_window_and_context();
}

X11GlxWindow::~X11GlxWindow() { destroy(); }

void X11GlxWindow::create_window_and_context() {
    display_ = XOpenDisplay(nullptr);
    if (!display_) throw std::runtime_error("Cannot open the X11 display. Set DISPLAY or run under XWayland/X11.");
    gl_module_ = dlopen("libGL.so.1", RTLD_NOW | RTLD_LOCAL);
    if (!gl_module_) throw std::runtime_error("Cannot load libGL.so.1");

    choose_fb_config = reinterpret_cast<ChooseFBConfigFn>(symbol(gl_module_, "glXChooseFBConfig"));
    get_visual_from_fb_config = reinterpret_cast<GetVisualFromFBConfigFn>(symbol(gl_module_, "glXGetVisualFromFBConfig"));
    create_new_context = reinterpret_cast<CreateNewContextFn>(symbol(gl_module_, "glXCreateNewContext"));
    make_current = reinterpret_cast<MakeCurrentFn>(symbol(gl_module_, "glXMakeCurrent"));
    swap_buffers = reinterpret_cast<SwapBuffersFn>(symbol(gl_module_, "glXSwapBuffers"));
    destroy_context = reinterpret_cast<DestroyContextFn>(symbol(gl_module_, "glXDestroyContext"));

    const int screen = DefaultScreen(display_);
    const std::array<int, 27> msaa_attributes{
        glx_x_renderable, True,
        glx_drawable_type, glx_window_bit,
        glx_render_type, glx_rgba_bit,
        glx_x_visual_type, glx_true_color,
        glx_red_size, 8, glx_green_size, 8, glx_blue_size, 8, glx_alpha_size, 8,
        glx_depth_size, 24, glx_stencil_size, 8,
        glx_doublebuffer, True,
        glx_sample_buffers, 1, glx_samples, 4,
        None
    };
    const std::array<int, 23> fallback_attributes{
        glx_x_renderable, True,
        glx_drawable_type, glx_window_bit,
        glx_render_type, glx_rgba_bit,
        glx_x_visual_type, glx_true_color,
        glx_red_size, 8, glx_green_size, 8, glx_blue_size, 8, glx_alpha_size, 8,
        glx_depth_size, 24, glx_stencil_size, 8,
        glx_doublebuffer, True,
        None
    };
    int config_count{};
    GLXFBConfig* configs = choose_fb_config(display_, screen, msaa_attributes.data(), &config_count);
    if (!configs || config_count == 0) {
        if (configs) XFree(configs);
        configs = choose_fb_config(display_, screen, fallback_attributes.data(), &config_count);
    }
    if (!configs || config_count == 0) throw std::runtime_error("No suitable GLX framebuffer configuration found");
    GLXFBConfig config = configs[0];
    XVisualInfo* visual = get_visual_from_fb_config(display_, config);
    if (!visual) { XFree(configs); throw std::runtime_error("GLX framebuffer has no X11 visual"); }

    XSetWindowAttributes attributes{};
    attributes.colormap = XCreateColormap(display_, RootWindow(display_, visual->screen), visual->visual, AllocNone);
    attributes.event_mask = ExposureMask | StructureNotifyMask | KeyPressMask | KeyReleaseMask
        | ButtonPressMask | ButtonReleaseMask | PointerMotionMask | FocusChangeMask;
    colormap_ = attributes.colormap;
    window_ = XCreateWindow(display_, RootWindow(display_, visual->screen),
        0, 0, static_cast<unsigned>(state_.width), static_cast<unsigned>(state_.height), 0,
        visual->depth, InputOutput, visual->visual, CWColormap | CWEventMask, &attributes);
    if (!window_) { XFree(visual); XFree(configs); throw std::runtime_error("X11 window creation failed"); }

    wm_delete_ = XInternAtom(display_, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(display_, window_, &wm_delete_, 1);
    XStoreName(display_, window_, "OpenGL Scene");

    create_context_attribs = reinterpret_cast<CreateContextAttribsFn>(render::gl::proc_address("glXCreateContextAttribsARB"));
    if (create_context_attribs) {
        int flags = 0;
#ifndef NDEBUG
        flags |= glx_context_debug_bit_arb;
#endif
        const int context_attributes[]{
            glx_context_major_version_arb, 4,
            glx_context_minor_version_arb, 5,
            glx_context_profile_mask_arb, glx_context_core_profile_bit_arb,
            glx_context_flags_arb, flags,
            None
        };
        context_ = create_context_attribs(display_, config, nullptr, True, context_attributes);
    }
    if (!context_) context_ = create_new_context(display_, config, glx_rgba_type, nullptr, True);
    XFree(visual);
    XFree(configs);
    if (!context_) throw std::runtime_error("OpenGL context creation failed through GLX");
    if (!make_current(display_, window_, context_)) throw std::runtime_error("glXMakeCurrent failed");

    render::gl::load_all();
    swap_interval_ext = reinterpret_cast<SwapIntervalExtFn>(render::gl::proc_address("glXSwapIntervalEXT"));
    swap_interval_mesa = reinterpret_cast<SwapIntervalMesaFn>(render::gl::proc_address("glXSwapIntervalMESA"));
    swap_interval_sgi = reinterpret_cast<SwapIntervalSgiFn>(render::gl::proc_address("glXSwapIntervalSGI"));

    arrow_cursor_ = XCreateFontCursor(display_, XC_left_ptr);
    resize_cursor_ = XCreateFontCursor(display_, XC_sb_h_double_arrow);
    text_cursor_ = XCreateFontCursor(display_, XC_xterm);
    blank_cursor_ = make_blank_cursor(display_, window_);
    XDefineCursor(display_, window_, arrow_cursor_);
}

void X11GlxWindow::destroy() noexcept {
    if (!display_) return;
    if (pointer_hidden_) XUngrabPointer(display_, CurrentTime);
    if (context_) {
        make_current(display_, None, nullptr);
        destroy_context(display_, context_);
        context_ = nullptr;
    }
    if (blank_cursor_) XFreeCursor(display_, blank_cursor_);
    if (text_cursor_) XFreeCursor(display_, text_cursor_);
    if (resize_cursor_) XFreeCursor(display_, resize_cursor_);
    if (arrow_cursor_) XFreeCursor(display_, arrow_cursor_);
    if (window_) XDestroyWindow(display_, window_);
    if (colormap_) XFreeColormap(display_, colormap_);
    XCloseDisplay(display_);
    display_ = nullptr;
    if (gl_module_) dlclose(gl_module_);
    gl_module_ = nullptr;
}

void X11GlxWindow::show() noexcept {
    XMapRaised(display_, window_);
    XFlush(display_);
}

void X11GlxWindow::handle_key(const XKeyEvent& event, bool pressed) noexcept {
    XKeyEvent mutable_event = event;
    const KeySym symbol_value = XLookupKeysym(&mutable_event, 0);
    const std::size_t key = portable_key(symbol_value);
    if (key < state_.input.keys.size()) state_.input.keys[key] = pressed;
    if (!pressed) return;
    if (symbol_value == XK_Escape) state_.running = false;
    switch (symbol_value) {
    case XK_F1: state_.controls.wireframe = !state_.controls.wireframe; break;
    case XK_F2: state_.controls.vsync = !state_.controls.vsync; break;
    case XK_F3: state_.controls.show_gui = !state_.controls.show_gui; break;
    case XK_F4: state_.controls.bloom = !state_.controls.bloom; break;
    case XK_F5: state_.controls.shadows = !state_.controls.shadows; break;
    case XK_F6: state_.controls.animation = !state_.controls.animation; break;
    case XK_F7: state_.controls.scene_debug_view = !state_.controls.scene_debug_view; break;
    case XK_F10: state_.controls.show_help = !state_.controls.show_help; break;
    default: break;
    }

    std::array<char, 16> text{};
    KeySym ignored{};
    const int count = XLookupString(&mutable_event, text.data(), static_cast<int>(text.size()), &ignored, nullptr);
    for (int index = 0; index < count && state_.input.text_input_count < state_.input.text_input.size(); ++index) {
        const unsigned char character = static_cast<unsigned char>(text[static_cast<std::size_t>(index)]);
        if (character == '\b' || character == '\r' || (character >= 32 && character <= 126))
            state_.input.text_input[state_.input.text_input_count++] = static_cast<char>(character);
    }
}

void X11GlxWindow::set_pointer_hidden(bool hidden) noexcept {
    if (hidden == pointer_hidden_) return;
    pointer_hidden_ = hidden;
    if (hidden) {
        XDefineCursor(display_, window_, blank_cursor_);
        XGrabPointer(display_, window_, True, PointerMotionMask | ButtonReleaseMask,
            GrabModeAsync, GrabModeAsync, window_, None, CurrentTime);
    } else {
        XUngrabPointer(display_, CurrentTime);
        XDefineCursor(display_, window_, arrow_cursor_);
    }
}

void X11GlxWindow::handle_button(const XButtonEvent& event, bool pressed) noexcept {
    if (event.button == Button1) {
        state_.input.left_mouse_down = pressed;
        if (pressed) state_.input.left_mouse_pressed = true;
        else state_.input.left_mouse_released = true;
    } else if (event.button == Button3) {
        state_.input.right_mouse_down = pressed;
        state_.input.have_last_mouse = false;
        set_pointer_hidden(pressed);
        if (pressed) {
            const int center_x = state_.width / 2;
            const int center_y = state_.height / 2;
            state_.input.mouse_x = center_x;
            state_.input.mouse_y = center_y;
            XWarpPointer(display_, None, window_, 0, 0, 0, 0, center_x, center_y);
            XFlush(display_);
        }
    } else if (pressed && event.button == Button4) {
        ++state_.input.mouse_wheel;
    } else if (pressed && event.button == Button5) {
        --state_.input.mouse_wheel;
    }
}

void X11GlxWindow::poll_events() noexcept {
    while (XPending(display_) > 0) {
        XEvent event{};
        XNextEvent(display_, &event);
        switch (event.type) {
        case ClientMessage:
            if (static_cast<Atom>(event.xclient.data.l[0]) == wm_delete_) state_.running = false;
            break;
        case ConfigureNotify:
            state_.width = std::max(1, event.xconfigure.width);
            state_.height = std::max(1, event.xconfigure.height);
            state_.resized = true;
            break;
        case KeyPress: handle_key(event.xkey, true); break;
        case KeyRelease:
            if (XEventsQueued(display_, QueuedAfterReading) > 0) {
                XEvent next{};
                XPeekEvent(display_, &next);
                if (next.type == KeyPress && next.xkey.time == event.xkey.time
                    && next.xkey.keycode == event.xkey.keycode) break;
            }
            handle_key(event.xkey, false);
            break;
        case ButtonPress: handle_button(event.xbutton, true); break;
        case ButtonRelease: handle_button(event.xbutton, false); break;
        case MotionNotify: {
            const int x = event.xmotion.x;
            const int y = event.xmotion.y;
            if (state_.input.right_mouse_down) {
                const int center_x = state_.width / 2;
                const int center_y = state_.height / 2;
                const int delta_x = x - center_x;
                const int delta_y = y - center_y;
                if (delta_x != 0 || delta_y != 0) {
                    state_.input.mouse_dx += delta_x;
                    state_.input.mouse_dy += delta_y;
                    XWarpPointer(display_, None, window_, 0, 0, 0, 0, center_x, center_y);
                    XFlush(display_);
                }
                state_.input.mouse_x = center_x;
                state_.input.mouse_y = center_y;
                state_.input.last_mouse_x = center_x;
                state_.input.last_mouse_y = center_y;
                state_.input.have_last_mouse = true;
            } else {
                state_.input.mouse_x = x;
                state_.input.mouse_y = y;
            }
            break;
        }
        case FocusOut:
            state_.input.keys.fill(false);
            state_.input.left_mouse_down = false;
            state_.input.right_mouse_down = false;
            set_pointer_hidden(false);
            break;
        default: break;
        }
    }
}

void X11GlxWindow::swap() const noexcept { swap_buffers(display_, window_); }

void X11GlxWindow::set_vsync(bool enabled) noexcept {
    const int interval = enabled ? 1 : 0;
    if (swap_interval_ext) swap_interval_ext(display_, window_, interval);
    else if (swap_interval_mesa) (void)swap_interval_mesa(static_cast<unsigned>(interval));
    else if (swap_interval_sgi && interval > 0) (void)swap_interval_sgi(interval);
    swap_interval_ = interval;
}

void X11GlxWindow::set_title(std::string_view title) const noexcept {
    const std::string copy{title};
    XStoreName(display_, window_, copy.c_str());
}

void X11GlxWindow::set_cursor(context::CursorShape shape) noexcept {
    if (pointer_hidden_) return;
    Cursor cursor = arrow_cursor_;
    if (shape == context::CursorShape::horizontal_resize) cursor = resize_cursor_;
    else if (shape == context::CursorShape::text) cursor = text_cursor_;
    XDefineCursor(display_, window_, cursor);
}

void* X11GlxWindow::native_handle() const noexcept {
    return reinterpret_cast<void*>(static_cast<std::uintptr_t>(window_));
}

} // namespace epoch::platform::linuxx
