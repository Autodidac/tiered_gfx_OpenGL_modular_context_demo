#pragma once

#include <chrono>

namespace epoch::core {
class FrameClock {
public:
    FrameClock() noexcept;
    [[nodiscard]] float tick() noexcept;
    [[nodiscard]] float elapsed_seconds() const noexcept { return elapsed_seconds_; }
private:
    using Clock = std::chrono::steady_clock;
    Clock::time_point last_;
    float elapsed_seconds_{};
};
}
