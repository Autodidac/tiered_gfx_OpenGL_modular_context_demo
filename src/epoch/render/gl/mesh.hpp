#pragma once

#include <cstdint>
#include <filesystem>
#include <span>
#include <vector>
#include "epoch/render/gl/gl_api.hpp"

#if defined(EPOCH_NO_MODULES)
#include "epoch/compat/epoch.core.math.hpp"
#else
import epoch.core.math;
#endif

namespace epoch::render::gl {

struct Vertex {
    math::Vec3 position{};
    math::Vec3 normal{};
    math::Vec2 uv{};
    math::Vec4 tangent{};
    math::Vec4 color{1.0f, 1.0f, 1.0f, 1.0f};
};

struct MeshData {
    std::vector<Vertex> vertices;
    std::vector<std::uint32_t> indices;
};

class Mesh {
public:
    Mesh(std::span<const Vertex> vertices, std::span<const std::uint32_t> indices);
    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;
    Mesh(Mesh&& other) noexcept;
    Mesh& operator=(Mesh&& other) noexcept;
    ~Mesh();
    void bind() const noexcept;
    void draw() const noexcept;
    void draw_instanced(GLsizei instance_count) const noexcept;
    void draw_indirect(const void* command_offset = nullptr) const noexcept;
    [[nodiscard]] GLsizei index_count() const noexcept { return index_count_; }

private:
    void destroy() noexcept;
    GLuint vao_{};
    GLuint vbo_{};
    GLuint ebo_{};
    GLsizei index_count_{};
};

[[nodiscard]] MeshData make_cube_mesh();
[[nodiscard]] MeshData make_plane_mesh(float uv_scale = 1.0f);
[[nodiscard]] MeshData make_billboard_mesh();
[[nodiscard]] MeshData make_gabled_roof_mesh();
[[nodiscard]] MeshData make_grid_mesh(int columns, int rows, float width, float depth, float uv_scale = 1.0f);
[[nodiscard]] MeshData load_obj(const std::filesystem::path& path);
void calculate_tangents(MeshData& data);

} // namespace epoch::render::gl
