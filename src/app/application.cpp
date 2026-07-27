module;
#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#elif defined(__linux__)
#include <unistd.h>
#endif
#include <array>
#include <cstdio>
#include <exception>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <utility>

module epoch.app;

import epoch.context.spine;
import epoch.render.spine;
import epoch.core.log;

namespace epoch::app {
namespace {

std::filesystem::path resolve_asset_root() {
#if defined(_WIN32)
    std::array<wchar_t, 32768> buffer{};
    const DWORD length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (length == 0 || length >= buffer.size())
        throw std::runtime_error("Cannot resolve executable path");
    const auto beside_executable = std::filesystem::path{buffer.data()}.parent_path() / "assets";
#elif defined(__linux__)
    std::array<char, 32768> buffer{};
    const auto length = ::readlink("/proc/self/exe", buffer.data(), buffer.size() - 1u);
    if (length <= 0 || static_cast<std::size_t>(length) >= buffer.size())
        throw std::runtime_error("Cannot resolve executable path");
    buffer[static_cast<std::size_t>(length)] = '\0';
    const auto beside_executable = std::filesystem::path{buffer.data()}.parent_path() / "assets";
#else
    const auto beside_executable = std::filesystem::current_path() / "assets";
#endif
    if (std::filesystem::exists(beside_executable / "shaders/pbr/pbr.frag")) return beside_executable;

    const auto local = std::filesystem::current_path() / "assets";
    if (std::filesystem::exists(local / "shaders/pbr/pbr.frag")) return local;

    throw std::runtime_error(
        "Asset directory not found. Build through CMake so assets are copied beside the executable.");
}

class Application final {
public:
    Application(context::NativeApplicationInstance instance, std::filesystem::path asset_root)
        : context_{instance}, renderer_{std::move(asset_root)} {}

    int run() {
        context::FrameContext frame{};
        float title_accumulator{};
        int title_frames{};
        while (context_.begin_frame(frame)) {
            context_.apply_runtime_controls();
            renderer_.update(context_.input(), frame, context_.controls());
            renderer_.render(frame, context_.controls());
            context_.end_frame();
            title_accumulator += frame.delta_seconds;
            ++title_frames;
            if (title_accumulator >= 0.35f) {
                context_.update_title(static_cast<float>(title_frames) / title_accumulator);
                title_accumulator = 0.0f;
                title_frames = 0;
            }
        }
        return 0;
    }

private:
    context::ContextSpine context_;
    render::RenderSpine renderer_;
};

} // namespace

int run_application(void* native_instance) {
    try {
        Application application{
            static_cast<context::NativeApplicationInstance>(native_instance),
            resolve_asset_root()
        };
        return application.run();
    } catch (const std::exception& error) {
        core::log_error(error.what());
#if defined(_WIN32)
        const std::string message = std::string{"Integrated OpenGL Scene failed:\n\n"} + error.what();
        MessageBoxA(nullptr, message.c_str(), "Integrated OpenGL Scene", MB_OK | MB_ICONERROR);
#else
        std::fprintf(stderr, "Integrated OpenGL Scene failed:\n\n%s\n", error.what());
#endif
        return 1;
    }
}

} // namespace epoch::app
