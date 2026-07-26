#include <android/log.h>
#include <android_native_app_glue.h>
#include <EGL/egl.h>
#include <GLES3/gl32.h>

#include <algorithm>
#include <array>
#include <cstdint>

namespace {
constexpr char log_tag[] = "EpochGui";

// Mobile Tier-0 render budget. This is a policy profile, not an OpenGL ES rule.
struct Tier0Budget {
    static constexpr std::uint32_t maximum_spotlights = 1;
    static constexpr std::uint32_t maximum_point_lights = 4;
    static constexpr std::uint32_t directional_shadow_taps = 9;
    static constexpr bool tessellation = false;
    static constexpr bool geometry_shaders = false;
    static constexpr bool pcss = false;
    static constexpr bool ssao = false;
    static constexpr bool indirect_draw = false;
};

struct AndroidEglShell {
    android_app* app{};
    EGLDisplay display{EGL_NO_DISPLAY};
    EGLSurface surface{EGL_NO_SURFACE};
    EGLContext context{EGL_NO_CONTEXT};
    int width{};
    int height{};
    bool ready{};

    bool create() {
        if (!app || !app->window) return false;
        display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        if (display == EGL_NO_DISPLAY || !eglInitialize(display, nullptr, nullptr)) return false;

        constexpr std::array<EGLint, 15> config_attributes{
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_RED_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_BLUE_SIZE, 8,
            EGL_ALPHA_SIZE, 8,
            EGL_DEPTH_SIZE, 24,
            EGL_NONE
        };
        EGLConfig config{};
        EGLint count{};
        if (!eglChooseConfig(display, config_attributes.data(), &config, 1, &count) || count == 0) return false;

        EGLint format{};
        eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format);
        ANativeWindow_setBuffersGeometry(app->window, 0, 0, format);

        surface = eglCreateWindowSurface(display, config, app->window, nullptr);
        constexpr std::array<EGLint, 3> context_attributes{EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
        context = eglCreateContext(display, config, EGL_NO_CONTEXT, context_attributes.data());
        if (surface == EGL_NO_SURFACE || context == EGL_NO_CONTEXT
            || !eglMakeCurrent(display, surface, surface, context)) return false;

        eglQuerySurface(display, surface, EGL_WIDTH, &width);
        eglQuerySurface(display, surface, EGL_HEIGHT, &height);
        eglSwapInterval(display, 1);
        ready = true;
        return true;
    }

    void destroy() {
        ready = false;
        if (display != EGL_NO_DISPLAY) {
            eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
            if (context != EGL_NO_CONTEXT) eglDestroyContext(display, context);
            if (surface != EGL_NO_SURFACE) eglDestroySurface(display, surface);
            eglTerminate(display);
        }
        display = EGL_NO_DISPLAY;
        surface = EGL_NO_SURFACE;
        context = EGL_NO_CONTEXT;
    }

    void frame() {
        if (!ready) return;
        // Replace this proof frame with the EPOCH_GLES renderer bridge. The bridge
        // should consume AAssetManager, touch/gamepad input and the Tier0Budget above.
        glViewport(0, 0, width, height);
        glClearColor(0.025f, 0.035f, 0.055f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        eglSwapBuffers(display, surface);
    }
};

void on_command(android_app* app, std::int32_t command) {
    auto& shell = *static_cast<AndroidEglShell*>(app->userData);
    switch (command) {
    case APP_CMD_INIT_WINDOW:
        if (app->window && !shell.create())
            __android_log_print(ANDROID_LOG_ERROR, log_tag, "EGL/OpenGL ES initialization failed");
        break;
    case APP_CMD_TERM_WINDOW:
        shell.destroy();
        break;
    default:
        break;
    }
}
} // namespace

void android_main(android_app* app) {
    app_dummy();
    AndroidEglShell shell{.app = app};
    app->userData = &shell;
    app->onAppCmd = on_command;

    while (!app->destroyRequested) {
        android_poll_source* source{};
        int events{};
        const int timeout = shell.ready ? 0 : -1;
        while (ALooper_pollOnce(timeout, nullptr, &events, reinterpret_cast<void**>(&source)) >= 0) {
            if (source) source->process(app, source);
            if (app->destroyRequested) break;
            if (timeout == 0) break;
        }
        shell.frame();
    }
    shell.destroy();
}
