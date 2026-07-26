#pragma once

#include <filesystem>
#include <initializer_list>
#include <span>
#include <string>
#include <string_view>
#include "epoch/render/gl/gl_api.hpp"

#if defined(EPOCH_NO_MODULES)
#include "epoch/compat/epoch.core.math.hpp"
#else
import epoch.core.math;
#endif

namespace epoch::render::gl {

struct ShaderStageSource {
    GLenum type{};
    std::filesystem::path path;
};

class ShaderProgram {
public:
    ShaderProgram() = default;
    ShaderProgram(const std::filesystem::path& vertex_path, const std::filesystem::path& fragment_path);
    explicit ShaderProgram(std::initializer_list<ShaderStageSource> stages);
    explicit ShaderProgram(std::span<const ShaderStageSource> stages);
    ShaderProgram(const ShaderProgram&) = delete;
    ShaderProgram& operator=(const ShaderProgram&) = delete;
    ShaderProgram(ShaderProgram&& other) noexcept;
    ShaderProgram& operator=(ShaderProgram&& other) noexcept;
    ~ShaderProgram();

    void bind() const noexcept;
    [[nodiscard]] GLuint id() const noexcept { return program_; }
    [[nodiscard]] GLint uniform(const char* name) const noexcept;
    void set(const char* name, const math::Mat4& value) const noexcept;
    void set_matrix_array(const char* name, std::span<const math::Mat4> values) const noexcept;
    void set(const char* name, math::Vec4 value) const noexcept;
    void set(const char* name, math::Vec3 value) const noexcept;
    void set(const char* name, math::Vec2 value) const noexcept;
    void set(const char* name, float value) const noexcept;
    void set(const char* name, int value) const noexcept;
    void set(const char* name, unsigned value) const noexcept;

private:
    static std::string read_text(const std::filesystem::path& path);
    static GLuint compile(GLenum type, std::string_view source, const std::filesystem::path& path);
    void link(std::span<const ShaderStageSource> stages);
    void destroy() noexcept;
    GLuint program_{};
};

} // namespace epoch::render::gl
