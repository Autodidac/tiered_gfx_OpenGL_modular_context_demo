#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include "epoch/platform/win32/win32_window.hpp"

namespace epoch::platform::win32 {

class WglContext {
public:
    explicit WglContext(const Window& window);
    WglContext(const WglContext&) = delete;
    WglContext& operator=(const WglContext&) = delete;
    ~WglContext();

    void swap() const noexcept;
    void set_vsync(bool enabled) const noexcept;
    [[nodiscard]] int swap_interval() const noexcept;

private:
    void load_extensions();
    void choose_real_pixel_format();
    void create_core_context();

    const Window& window_;
    HGLRC context_{};
};

} // namespace epoch::platform::win32
