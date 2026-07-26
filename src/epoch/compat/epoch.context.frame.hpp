#pragma once

namespace epoch::context {
struct FrameContext {
    float delta_seconds{};
    float elapsed_seconds{};
    int framebuffer_width{1};
    int framebuffer_height{1};
    bool resized{};
};
}
