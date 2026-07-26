#pragma once

#include <array>
#include <filesystem>
#include "epoch/render/gl/gl_api.hpp"

namespace epoch::render::gl {

enum class ColorSpace { linear, srgb };

class Texture2D {
public:
    Texture2D(const std::filesystem::path& path, ColorSpace color_space, float anisotropy, bool flip_y = true,
              bool mipmaps = true, GLenum wrap = GL_REPEAT);
    Texture2D(int width, int height, GLenum internal_format, GLenum format, GLenum type, const void* pixels,
              bool mipmaps, GLenum wrap = GL_REPEAT, float anisotropy = 1.0f);
    Texture2D(const Texture2D&) = delete;
    Texture2D& operator=(const Texture2D&) = delete;
    Texture2D(Texture2D&& other) noexcept;
    Texture2D& operator=(Texture2D&& other) noexcept;
    ~Texture2D();

    void bind(int unit) const noexcept;
    [[nodiscard]] GLuint id() const noexcept { return id_; }
    [[nodiscard]] int width() const noexcept { return width_; }
    [[nodiscard]] int height() const noexcept { return height_; }

private:
    void destroy() noexcept;
    GLuint id_{};
    int width_{};
    int height_{};
};

class TextureCube {
public:
    TextureCube(const std::array<std::filesystem::path, 6>& faces, ColorSpace color_space, bool flip_y = false);
    TextureCube(const TextureCube&) = delete;
    TextureCube& operator=(const TextureCube&) = delete;
    TextureCube(TextureCube&& other) noexcept;
    TextureCube& operator=(TextureCube&& other) noexcept;
    ~TextureCube();

    void bind(int unit) const noexcept;
    [[nodiscard]] GLuint id() const noexcept { return id_; }

private:
    void destroy() noexcept;
    GLuint id_{};
};

} // namespace epoch::render::gl
