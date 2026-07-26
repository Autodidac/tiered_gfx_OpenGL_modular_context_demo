#include "epoch/render/gl/texture.hpp"
#include "epoch/render/gl/image.hpp"

#include <stdexcept>
#include <utility>

namespace epoch::render::gl {
namespace {
struct Formats {
    GLenum format{};
    GLenum internal{};
};

Formats formats_for(const Image& image, ColorSpace color_space) {
    if (image.channels == 1) return {red, r8};
    if (image.channels == 3) return {GL_RGB, color_space == ColorSpace::srgb ? srgb8 : rgb8};
    if (image.channels == 4) return {GL_RGBA, color_space == ColorSpace::srgb ? srgb8_alpha8 : rgba8};
    throw std::runtime_error("Unsupported image channel count");
}
}

Texture2D::Texture2D(const std::filesystem::path& path, ColorSpace color_space, float anisotropy, bool flip_y,
                     bool mipmaps, GLenum wrap) {
    const Image image = load_image(path, flip_y);
    const auto [format, internal] = formats_for(image, color_space);
    width_ = image.width;
    height_ = image.height;
    gl::GenTextures(1, &id_);
    gl::BindTexture(GL_TEXTURE_2D, id_);
    gl::PixelStorei(GL_UNPACK_ALIGNMENT, 1);
    gl::TexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(internal), width_, height_, 0,
                 format, GL_UNSIGNED_BYTE, image.pixels.data());
    gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, mipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
    gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
    gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);
    if (anisotropy > 1.0f && mipmaps) gl::TexParameterf(GL_TEXTURE_2D, texture_max_anisotropy_ext, anisotropy);
    if (mipmaps) GenerateMipmap(GL_TEXTURE_2D);
}

Texture2D::Texture2D(int width, int height, GLenum internal_format, GLenum format, GLenum type, const void* pixels,
                     bool mipmaps, GLenum wrap, float anisotropy) : width_{width}, height_{height} {
    gl::GenTextures(1, &id_);
    gl::BindTexture(GL_TEXTURE_2D, id_);
    gl::TexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(internal_format), width, height, 0, format, type, pixels);
    gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, mipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
    gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
    gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);
    if (anisotropy > 1.0f && mipmaps) gl::TexParameterf(GL_TEXTURE_2D, texture_max_anisotropy_ext, anisotropy);
    if (mipmaps) GenerateMipmap(GL_TEXTURE_2D);
}

Texture2D::Texture2D(Texture2D&& other) noexcept
    : id_{std::exchange(other.id_,0)}, width_{std::exchange(other.width_,0)}, height_{std::exchange(other.height_,0)} {}
Texture2D& Texture2D::operator=(Texture2D&& other) noexcept {
    if(this!=&other){destroy();id_=std::exchange(other.id_,0);width_=std::exchange(other.width_,0);height_=std::exchange(other.height_,0);}return *this;
}
Texture2D::~Texture2D(){destroy();}
void Texture2D::destroy() noexcept { if(id_) gl::DeleteTextures(1,&id_); id_=0; width_=height_=0; }
void Texture2D::bind(int unit) const noexcept { ActiveTexture(texture0+unit); gl::BindTexture(GL_TEXTURE_2D,id_); }

TextureCube::TextureCube(const std::array<std::filesystem::path, 6>& faces, ColorSpace color_space, bool flip_y) {
    gl::GenTextures(1, &id_);
    gl::BindTexture(texture_cube_map, id_);
    int expected_width{};
    int expected_height{};
    for (std::size_t index = 0; index < faces.size(); ++index) {
        const Image image = load_image(faces[index], flip_y);
        const auto [format, internal] = formats_for(image, color_space);
        if (index == 0) { expected_width = image.width; expected_height = image.height; }
        if (image.width != expected_width || image.height != expected_height)
            throw std::runtime_error("Cubemap faces must have matching dimensions");
        gl::TexImage2D(texture_cube_map_positive_x + static_cast<GLenum>(index), 0,
                     static_cast<GLint>(internal), image.width, image.height, 0,
                     format, GL_UNSIGNED_BYTE, image.pixels.data());
    }
    gl::TexParameteri(texture_cube_map, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    gl::TexParameteri(texture_cube_map, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    gl::TexParameteri(texture_cube_map, GL_TEXTURE_WRAP_S, clamp_to_edge);
    gl::TexParameteri(texture_cube_map, GL_TEXTURE_WRAP_T, clamp_to_edge);
    gl::TexParameteri(texture_cube_map, texture_wrap_r, clamp_to_edge);
    GenerateMipmap(texture_cube_map);
}

TextureCube::TextureCube(TextureCube&& other) noexcept : id_{std::exchange(other.id_, 0)} {}
TextureCube& TextureCube::operator=(TextureCube&& other) noexcept {
    if (this != &other) { destroy(); id_ = std::exchange(other.id_, 0); }
    return *this;
}
TextureCube::~TextureCube() { destroy(); }
void TextureCube::destroy() noexcept { if (id_) gl::DeleteTextures(1, &id_); id_ = 0; }
void TextureCube::bind(int unit) const noexcept { ActiveTexture(texture0 + unit); gl::BindTexture(texture_cube_map, id_); }

} // namespace epoch::render::gl
