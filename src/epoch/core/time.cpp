module;
#include <algorithm>
#include <chrono>

module epoch.core.time;

namespace epoch::core {

FrameClock::FrameClock() noexcept : last_{Clock::now()} {}

float FrameClock::tick() noexcept {
    const auto now = Clock::now();
    const float dt = std::chrono::duration<float>(now - last_).count();
    last_ = now;
    const float clamped = std::clamp(dt, 0.0f, 0.1f);
    elapsed_seconds_ += clamped;
    return clamped;
}

} // namespace epoch::core
