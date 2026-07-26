#include "epoch/render/gl/shader_program.hpp"
#include <algorithm>
#include <array>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <vector>

namespace epoch::render::gl {

ShaderProgram::ShaderProgram(const std::filesystem::path& vertex_path, const std::filesystem::path& fragment_path) {
    const std::array stages{
        ShaderStageSource{vertex_shader, vertex_path},
        ShaderStageSource{fragment_shader, fragment_path}
    };
    link(stages);
}

ShaderProgram::ShaderProgram(std::initializer_list<ShaderStageSource> stages) { link(stages); }
ShaderProgram::ShaderProgram(std::span<const ShaderStageSource> stages) { link(stages); }

void ShaderProgram::link(std::span<const ShaderStageSource> stages) {
    if (stages.empty()) throw std::runtime_error("Shader program requires at least one stage");
    std::vector<GLuint> compiled;
    compiled.reserve(stages.size());
    try {
        for (const auto& stage : stages)
            compiled.push_back(compile(stage.type, read_text(stage.path), stage.path));
        program_ = CreateProgram();
        for (const GLuint shader : compiled) AttachShader(program_, shader);
        LinkProgram(program_);
        for (const GLuint shader : compiled) DeleteShader(shader);
        compiled.clear();
        GLint linked{}; GetProgramiv(program_, link_status, &linked);
        if (!linked) {
            GLint length{}; GetProgramiv(program_, info_log_length, &length);
            std::string log(static_cast<std::size_t>(std::max(1, length)), '\0');
            GetProgramInfoLog(program_, length, nullptr, log.data());
            destroy();
            throw std::runtime_error("Program link failed:\n" + log);
        }
    } catch (...) {
        for (const GLuint shader : compiled) DeleteShader(shader);
        destroy();
        throw;
    }
}

ShaderProgram::ShaderProgram(ShaderProgram&& other) noexcept : program_{std::exchange(other.program_, 0)} {}
ShaderProgram& ShaderProgram::operator=(ShaderProgram&& other) noexcept {
    if (this != &other) { destroy(); program_ = std::exchange(other.program_, 0); }
    return *this;
}
ShaderProgram::~ShaderProgram() { destroy(); }
void ShaderProgram::destroy() noexcept { if (program_) DeleteProgram(program_); program_ = 0; }
void ShaderProgram::bind() const noexcept { UseProgram(program_); }
GLint ShaderProgram::uniform(const char* name) const noexcept { return GetUniformLocation(program_, name); }
void ShaderProgram::set(const char* name, const math::Mat4& v) const noexcept { const GLint l=uniform(name); if(l>=0) UniformMatrix4fv(l,1,GL_FALSE,v.data()); }
void ShaderProgram::set_matrix_array(const char* name, std::span<const math::Mat4> values) const noexcept {
    const GLint location = uniform(name);
    if (location >= 0 && !values.empty()) UniformMatrix4fv(location, static_cast<GLsizei>(values.size()), GL_FALSE, values.front().data());
}
void ShaderProgram::set(const char* name, math::Vec4 v) const noexcept { const GLint l=uniform(name); if(l>=0) Uniform4f(l,v.x,v.y,v.z,v.w); }
void ShaderProgram::set(const char* name, math::Vec3 v) const noexcept { const GLint l=uniform(name); if(l>=0) Uniform3f(l,v.x,v.y,v.z); }
void ShaderProgram::set(const char* name, math::Vec2 v) const noexcept { const GLint l=uniform(name); if(l>=0) Uniform2f(l,v.x,v.y); }
void ShaderProgram::set(const char* name, float v) const noexcept { const GLint l=uniform(name); if(l>=0) Uniform1f(l,v); }
void ShaderProgram::set(const char* name, int v) const noexcept { const GLint l=uniform(name); if(l>=0) Uniform1i(l,v); }
void ShaderProgram::set(const char* name, unsigned v) const noexcept { const GLint l=uniform(name); if(l>=0 && Uniform1ui) Uniform1ui(l,v); }

std::string ShaderProgram::read_text(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) throw std::runtime_error("Cannot open shader: " + path.string());
    std::ostringstream stream; stream << file.rdbuf(); return stream.str();
}

GLuint ShaderProgram::compile(GLenum type, std::string_view source, const std::filesystem::path& path) {
    const GLuint shader = CreateShader(type);
    const char* text = source.data(); const GLint length = static_cast<GLint>(source.size());
    ShaderSource(shader, 1, &text, &length); CompileShader(shader);
    GLint compiled{}; GetShaderiv(shader, compile_status, &compiled);
    if (!compiled) {
        GLint log_length{}; GetShaderiv(shader, info_log_length, &log_length);
        std::string log(static_cast<std::size_t>(std::max(1, log_length)), '\0');
        GetShaderInfoLog(shader, log_length, nullptr, log.data()); DeleteShader(shader);
        throw std::runtime_error("Shader compile failed: " + path.string() + "\n" + log);
    }
    return shader;
}

} // namespace epoch::render::gl
