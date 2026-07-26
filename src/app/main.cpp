#include <exception>
#include <iostream>
#include <string>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

import epoch.app;
import epoch.core.log;

namespace {
int run_application(void* native_instance) {
    try {
        epoch::app::Application application{
            static_cast<epoch::context::NativeApplicationInstance>(native_instance),
            epoch::app::resolve_asset_root()
        };
        return application.run();
    } catch (const std::exception& error) {
        epoch::core::log_error(error.what());
#if defined(_WIN32)
        const std::string message = std::string{"Integrated OpenGL Scene failed:\n\n"} + error.what();
        MessageBoxA(nullptr, message.c_str(), "Integrated OpenGL Scene", MB_OK | MB_ICONERROR);
#else
        std::cerr << "Integrated OpenGL Scene failed:\n\n" << error.what() << '\n';
#endif
        return 1;
    }
}
}

#if defined(_WIN32)
int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int) {
    return run_application(static_cast<void*>(instance));
}
#else
int main() {
    return run_application(nullptr);
}
#endif
