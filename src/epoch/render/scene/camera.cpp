module;
#include <algorithm>
#include <cmath>

module epoch.render.scene;

namespace epoch::render::scene {

math::Vec3 Camera::forward() const noexcept {
    const float yaw = math::radians(yaw_degrees);
    const float pitch = math::radians(pitch_degrees);
    return math::normalize({std::cos(pitch) * std::cos(yaw), std::sin(pitch), std::cos(pitch) * std::sin(yaw)});
}

math::Mat4 Camera::view() const noexcept { return math::look_at(position, position + forward(), {0, 1, 0}); }
math::Mat4 Camera::projection(float aspect) const noexcept { return math::perspective(math::radians(fov_degrees), aspect, near_plane, far_plane); }

void Camera::update(context::InputState& input, float dt) {
    yaw_degrees += static_cast<float>(input.mouse_dx) * 0.11f;
    pitch_degrees -= static_cast<float>(input.mouse_dy) * 0.11f;
    pitch_degrees = std::clamp(pitch_degrees, -88.0f, 88.0f);
    input.mouse_dx = 0; input.mouse_dy = 0;

    const math::Vec3 f = forward();
    const math::Vec3 flat_forward = math::normalize({f.x, 0.0f, f.z});
    const math::Vec3 right = math::normalize(math::cross(flat_forward, {0, 1, 0}));
    float velocity = speed * dt;
    if (input.keys[16]) velocity *= 2.8f;
    if (input.keys['W']) position += flat_forward * velocity;
    if (input.keys['S']) position -= flat_forward * velocity;
    if (input.keys['D']) position += right * velocity;
    if (input.keys['A']) position -= right * velocity;
    if (input.keys['E']) position.y += velocity;
    if (input.keys['Q']) position.y -= velocity;
}

} // namespace epoch::render::scene
