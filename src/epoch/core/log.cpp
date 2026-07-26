module;
#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif
#include <cstdio>
#include <string>
#include <string_view>

module epoch.core.log;

namespace epoch::core {
namespace {
void emit(const char* prefix, std::string_view message) noexcept {
    std::string line;
    line.reserve(message.size() + 24);
    line.append(prefix).append(message).append("\n");
#if defined(_WIN32)
    OutputDebugStringA(line.c_str());
#endif
    std::fputs(line.c_str(), stderr);
}
}
void log_info(std::string_view message) noexcept { emit("[epoch] ", message); }
void log_error(std::string_view message) noexcept { emit("[epoch:error] ", message); }
}
