#pragma once

#include <array>
#include "epoch/render/gl/gl_api.hpp"

namespace epoch::render::techniques {

class GpuProfiler {
public:
    GpuProfiler();
    GpuProfiler(const GpuProfiler&) = delete;
    GpuProfiler& operator=(const GpuProfiler&) = delete;
    ~GpuProfiler();

    void begin(bool enabled) noexcept;
    void end() noexcept;
    [[nodiscard]] float latest_milliseconds() const noexcept { return latest_milliseconds_; }

private:
    std::array<GLuint, 2> queries_{};
    std::size_t write_index_{};
    bool active_{};
    bool issued_previous_{};
    float latest_milliseconds_{};
};

} // namespace epoch::render::techniques
