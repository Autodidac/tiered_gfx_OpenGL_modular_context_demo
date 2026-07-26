#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>

namespace epoch::math {

struct Vec2 {
    float x{};
    float y{};
};

struct Vec3 {
    float x{};
    float y{};
    float z{};
};

struct Vec4 {
    float x{};
    float y{};
    float z{};
    float w{};
};

[[nodiscard]] constexpr Vec2 operator+(Vec2 a, Vec2 b) noexcept { return {a.x + b.x, a.y + b.y}; }
[[nodiscard]] constexpr Vec2 operator-(Vec2 a, Vec2 b) noexcept { return {a.x - b.x, a.y - b.y}; }
[[nodiscard]] constexpr Vec2 operator*(Vec2 v, float s) noexcept { return {v.x * s, v.y * s}; }
[[nodiscard]] constexpr Vec3 operator+(Vec3 a, Vec3 b) noexcept { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
[[nodiscard]] constexpr Vec3 operator-(Vec3 a, Vec3 b) noexcept { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
[[nodiscard]] constexpr Vec3 operator-(Vec3 v) noexcept { return {-v.x, -v.y, -v.z}; }
[[nodiscard]] constexpr Vec3 operator*(Vec3 v, float s) noexcept { return {v.x * s, v.y * s, v.z * s}; }
[[nodiscard]] constexpr Vec3 operator*(float s, Vec3 v) noexcept { return v * s; }
[[nodiscard]] constexpr Vec3 operator/(Vec3 v, float s) noexcept { return {v.x / s, v.y / s, v.z / s}; }
[[nodiscard]] constexpr Vec3 operator*(Vec3 a, Vec3 b) noexcept { return {a.x * b.x, a.y * b.y, a.z * b.z}; }

constexpr Vec3& operator+=(Vec3& a, Vec3 b) noexcept { a = a + b; return a; }
constexpr Vec3& operator-=(Vec3& a, Vec3 b) noexcept { a = a - b; return a; }

[[nodiscard]] constexpr float dot(Vec3 a, Vec3 b) noexcept { return a.x * b.x + a.y * b.y + a.z * b.z; }
[[nodiscard]] constexpr Vec3 cross(Vec3 a, Vec3 b) noexcept {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
[[nodiscard]] inline float length(Vec3 v) noexcept { return std::sqrt(dot(v, v)); }
[[nodiscard]] inline Vec3 normalize(Vec3 v) noexcept {
    const float len = length(v);
    return len > 1.0e-7f ? v / len : Vec3{};
}
[[nodiscard]] constexpr Vec3 lerp(Vec3 a, Vec3 b, float t) noexcept { return a + (b - a) * t; }
[[nodiscard]] constexpr float saturate(float v) noexcept { return std::clamp(v, 0.0f, 1.0f); }

struct Mat4 {
    std::array<float, 16> m{};

    [[nodiscard]] static constexpr Mat4 identity() noexcept {
        return {{1, 0, 0, 0,
                 0, 1, 0, 0,
                 0, 0, 1, 0,
                 0, 0, 0, 1}};
    }

    [[nodiscard]] constexpr const float* data() const noexcept { return m.data(); }
};

[[nodiscard]] constexpr Mat4 operator*(const Mat4& a, const Mat4& b) noexcept {
    Mat4 result{};
    for (int column = 0; column < 4; ++column) {
        for (int row = 0; row < 4; ++row) {
            float value = 0.0f;
            for (int k = 0; k < 4; ++k) value += a.m[k * 4 + row] * b.m[column * 4 + k];
            result.m[column * 4 + row] = value;
        }
    }
    return result;
}


[[nodiscard]] constexpr Vec4 operator*(const Mat4& matrix, Vec4 vector) noexcept {
    return {
        matrix.m[0] * vector.x + matrix.m[4] * vector.y + matrix.m[8] * vector.z + matrix.m[12] * vector.w,
        matrix.m[1] * vector.x + matrix.m[5] * vector.y + matrix.m[9] * vector.z + matrix.m[13] * vector.w,
        matrix.m[2] * vector.x + matrix.m[6] * vector.y + matrix.m[10] * vector.z + matrix.m[14] * vector.w,
        matrix.m[3] * vector.x + matrix.m[7] * vector.y + matrix.m[11] * vector.z + matrix.m[15] * vector.w
    };
}

[[nodiscard]] constexpr Mat4 translate(Vec3 v) noexcept {
    Mat4 result = Mat4::identity();
    result.m[12] = v.x; result.m[13] = v.y; result.m[14] = v.z;
    return result;
}

[[nodiscard]] constexpr Mat4 scale(Vec3 v) noexcept {
    Mat4 result{};
    result.m[0] = v.x; result.m[5] = v.y; result.m[10] = v.z; result.m[15] = 1.0f;
    return result;
}

[[nodiscard]] inline Mat4 rotate_x(float radians) noexcept {
    const float c = std::cos(radians); const float s = std::sin(radians);
    return {{1, 0, 0, 0,
             0, c, s, 0,
             0, -s, c, 0,
             0, 0, 0, 1}};
}

[[nodiscard]] inline Mat4 rotate_y(float radians) noexcept {
    const float c = std::cos(radians); const float s = std::sin(radians);
    return {{c, 0, -s, 0,
             0, 1, 0, 0,
             s, 0, c, 0,
             0, 0, 0, 1}};
}

[[nodiscard]] inline Mat4 rotate_z(float radians) noexcept {
    const float c = std::cos(radians); const float s = std::sin(radians);
    return {{c, s, 0, 0,
             -s, c, 0, 0,
             0, 0, 1, 0,
             0, 0, 0, 1}};
}

[[nodiscard]] inline Mat4 perspective(float fov_y_radians, float aspect, float z_near, float z_far) noexcept {
    const float f = 1.0f / std::tan(fov_y_radians * 0.5f);
    Mat4 result{};
    result.m[0] = f / aspect;
    result.m[5] = f;
    result.m[10] = (z_far + z_near) / (z_near - z_far);
    result.m[11] = -1.0f;
    result.m[14] = (2.0f * z_far * z_near) / (z_near - z_far);
    return result;
}

[[nodiscard]] constexpr Mat4 ortho(float left, float right, float bottom, float top, float z_near, float z_far) noexcept {
    Mat4 result = Mat4::identity();
    result.m[0] = 2.0f / (right - left);
    result.m[5] = 2.0f / (top - bottom);
    result.m[10] = -2.0f / (z_far - z_near);
    result.m[12] = -(right + left) / (right - left);
    result.m[13] = -(top + bottom) / (top - bottom);
    result.m[14] = -(z_far + z_near) / (z_far - z_near);
    return result;
}

[[nodiscard]] inline Mat4 look_at(Vec3 eye, Vec3 center, Vec3 up) noexcept {
    const Vec3 f = normalize(center - eye);
    const Vec3 s = normalize(cross(f, up));
    const Vec3 u = cross(s, f);
    Mat4 result = Mat4::identity();
    result.m[0] = s.x; result.m[1] = u.x; result.m[2] = -f.x;
    result.m[4] = s.y; result.m[5] = u.y; result.m[6] = -f.y;
    result.m[8] = s.z; result.m[9] = u.z; result.m[10] = -f.z;
    result.m[12] = -dot(s, eye); result.m[13] = -dot(u, eye); result.m[14] = dot(f, eye);
    return result;
}

[[nodiscard]] inline Mat4 transform(Vec3 position, Vec3 rotation_radians, Vec3 scaling) noexcept {
    return translate(position) * rotate_y(rotation_radians.y) * rotate_x(rotation_radians.x) * rotate_z(rotation_radians.z) * scale(scaling);
}

inline constexpr float radians(float degrees) noexcept {
    return degrees * std::numbers::pi_v<float> / 180.0f;
}

} // namespace epoch::math
