#pragma once

#include <array>
#include "epoch/render/gl/gl_api.hpp"

namespace epoch::render::gl {

class ShadowTarget {
public:
    explicit ShadowTarget(int size = 2048);
    ShadowTarget(const ShadowTarget&) = delete;
    ShadowTarget& operator=(const ShadowTarget&) = delete;
    ~ShadowTarget();
    void bind_for_write() const noexcept;
    void bind_depth(int unit) const noexcept;
    [[nodiscard]] int size() const noexcept { return size_; }

private:
    int size_{};
    GLuint fbo_{};
    GLuint depth_{};
};

class HdrTarget {
public:
    HdrTarget() = default;
    HdrTarget(const HdrTarget&) = delete;
    HdrTarget& operator=(const HdrTarget&) = delete;
    ~HdrTarget();
    void resize(int width, int height);
    void bind_for_write() const noexcept;
    void bind_scene(int unit) const noexcept;
    void bind_bright(int unit) const noexcept;
    void bind_depth(int unit) const noexcept;
    [[nodiscard]] int width() const noexcept { return width_; }
    [[nodiscard]] int height() const noexcept { return height_; }

private:
    void destroy() noexcept;
    int width_{};
    int height_{};
    GLuint fbo_{};
    std::array<GLuint,2> color_{};
    GLuint depth_{};
};

class BlurTargets {
public:
    BlurTargets() = default;
    BlurTargets(const BlurTargets&) = delete;
    BlurTargets& operator=(const BlurTargets&) = delete;
    ~BlurTargets();
    void resize(int width, int height);
    void bind_for_write(int index) const noexcept;
    void bind_texture(int index, int unit) const noexcept;

private:
    void destroy() noexcept;
    int width_{};
    int height_{};
    std::array<GLuint,2> fbo_{};
    std::array<GLuint,2> texture_{};
};

} // namespace epoch::render::gl
