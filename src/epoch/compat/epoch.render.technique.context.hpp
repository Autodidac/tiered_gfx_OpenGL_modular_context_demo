#pragma once

#include "epoch/compat/epoch.core.math.hpp"
#include "epoch/compat/epoch.context.input.hpp"
namespace epoch::render::techniques {

struct TechniqueContext {
    math::Mat4 view{};
    math::Mat4 projection{};
    math::Mat4 view_projection{};
    math::Mat4 light_view_projection{};
    math::Mat4 projector_view_projection{};
    math::Vec3 camera_position{};
    math::Vec3 camera_forward{};
    math::Vec3 camera_right{};
    math::Vec3 camera_up{0.0f, 1.0f, 0.0f};
    int width{1};
    int height{1};
    float elapsed_seconds{};
    float camera_near{0.08f};
    float camera_far{180.0f};
    context::RuntimeControls controls{};
};

} // namespace epoch::render::techniques
