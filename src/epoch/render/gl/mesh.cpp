#include "epoch/render/gl/mesh.hpp"
#include <algorithm>
#include <cstddef>
#include <initializer_list>
#include <utility>

namespace epoch::render::gl {

Mesh::Mesh(std::span<const Vertex> vertices, std::span<const std::uint32_t> indices)
    : index_count_{static_cast<GLsizei>(indices.size())} {
    GenVertexArrays(1, &vao_); GenBuffers(1, &vbo_); GenBuffers(1, &ebo_);
    BindVertexArray(vao_);
    BindBuffer(array_buffer, vbo_); BufferData(array_buffer, static_cast<GLsizeiptr>(vertices.size_bytes()), vertices.data(), static_draw);
    BindBuffer(element_array_buffer, ebo_); BufferData(element_array_buffer, static_cast<GLsizeiptr>(indices.size_bytes()), indices.data(), static_draw);
    constexpr GLsizei stride = sizeof(Vertex);
    EnableVertexAttribArray(0); VertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(offsetof(Vertex, position)));
    EnableVertexAttribArray(1); VertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(offsetof(Vertex, normal)));
    EnableVertexAttribArray(2); VertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(offsetof(Vertex, uv)));
    EnableVertexAttribArray(3); VertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(offsetof(Vertex, tangent)));
    EnableVertexAttribArray(4); VertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(offsetof(Vertex, color)));
    BindVertexArray(0);
}

Mesh::Mesh(Mesh&& other) noexcept
    : vao_{std::exchange(other.vao_,0)}, vbo_{std::exchange(other.vbo_,0)}, ebo_{std::exchange(other.ebo_,0)}, index_count_{std::exchange(other.index_count_,0)} {}
Mesh& Mesh::operator=(Mesh&& other) noexcept {
    if(this!=&other){destroy();vao_=std::exchange(other.vao_,0);vbo_=std::exchange(other.vbo_,0);ebo_=std::exchange(other.ebo_,0);index_count_=std::exchange(other.index_count_,0);} return *this;
}
Mesh::~Mesh(){destroy();}
void Mesh::destroy() noexcept { if(ebo_)DeleteBuffers(1,&ebo_); if(vbo_)DeleteBuffers(1,&vbo_); if(vao_)DeleteVertexArrays(1,&vao_); vao_=vbo_=ebo_=0; index_count_=0; }
void Mesh::bind() const noexcept { BindVertexArray(vao_); }
void Mesh::draw() const noexcept { bind(); gl::DrawElements(GL_TRIANGLES,index_count_,GL_UNSIGNED_INT,nullptr); }
void Mesh::draw_instanced(GLsizei instance_count) const noexcept { bind(); DrawElementsInstanced(GL_TRIANGLES,index_count_,GL_UNSIGNED_INT,nullptr,instance_count); }
void Mesh::draw_indirect(const void* command_offset) const noexcept { bind(); DrawElementsIndirect(GL_TRIANGLES,GL_UNSIGNED_INT,command_offset); }

MeshData make_cube_mesh() {
    MeshData data; data.vertices.reserve(24); data.indices.reserve(36);
    const auto face=[&](math::Vec3 a, math::Vec3 b, math::Vec3 c, math::Vec3 d, math::Vec3 n, math::Vec4 t){
        const std::uint32_t base=static_cast<std::uint32_t>(data.vertices.size());
        data.vertices.push_back({a,n,{0,0},t});data.vertices.push_back({b,n,{1,0},t});data.vertices.push_back({c,n,{1,1},t});data.vertices.push_back({d,n,{0,1},t});
        data.indices.insert(data.indices.end(),{base,base+1,base+2,base,base+2,base+3});
    };
    constexpr float h=.5f;
    face({-h,-h,h},{h,-h,h},{h,h,h},{-h,h,h},{0,0,1},{1,0,0,1});
    face({h,-h,-h},{-h,-h,-h},{-h,h,-h},{h,h,-h},{0,0,-1},{-1,0,0,1});
    face({-h,-h,-h},{-h,-h,h},{-h,h,h},{-h,h,-h},{-1,0,0},{0,0,1,1});
    face({h,-h,h},{h,-h,-h},{h,h,-h},{h,h,h},{1,0,0},{0,0,-1,1});
    face({-h,h,h},{h,h,h},{h,h,-h},{-h,h,-h},{0,1,0},{1,0,0,1});
    face({-h,-h,-h},{h,-h,-h},{h,-h,h},{-h,-h,h},{0,-1,0},{1,0,0,-1});
    return data;
}

MeshData make_plane_mesh(float uv_scale) {
    MeshData data;
    data.vertices={{{-1,0,-1},{0,1,0},{0,0},{1,0,0,-1}},{{1,0,-1},{0,1,0},{uv_scale,0},{1,0,0,-1}},{{1,0,1},{0,1,0},{uv_scale,uv_scale},{1,0,0,-1}},{{-1,0,1},{0,1,0},{0,uv_scale},{1,0,0,-1}}};
    data.indices={0,2,1,0,3,2}; return data;
}

MeshData make_billboard_mesh() {
    MeshData data;
    data.vertices={{{-0.5f,0.0f,0.0f},{0,0,1},{0,0},{1,0,0,1}},{{0.5f,0.0f,0.0f},{0,0,1},{1,0},{1,0,0,1}},{{0.5f,1.0f,0.0f},{0,0,1},{1,1},{1,0,0,1}},{{-0.5f,1.0f,0.0f},{0,0,1},{0,1},{1,0,0,1}}};
    data.indices={0,1,2,0,2,3};
    return data;
}



MeshData make_gabled_roof_mesh() {
    MeshData data;
    const auto face = [&](std::initializer_list<math::Vec3> positions,
                          math::Vec3 normal,
                          std::initializer_list<math::Vec2> uvs) {
        const std::uint32_t base = static_cast<std::uint32_t>(data.vertices.size());
        auto uv = uvs.begin();
        for (const auto position : positions) {
            data.vertices.push_back({position, normal, *uv++, {}});
        }
        if (positions.size() == 4) {
            data.indices.insert(data.indices.end(), {base, base + 1, base + 2, base, base + 2, base + 3});
        } else {
            data.indices.insert(data.indices.end(), {base, base + 1, base + 2});
        }
    };

    const math::Vec3 positive_slope = math::normalize(math::Vec3{0.0f, 1.0f, 1.0f});
    const math::Vec3 negative_slope = math::normalize(math::Vec3{0.0f, 1.0f, -1.0f});
    face({{-0.5f,0.0f,0.5f},{0.5f,0.0f,0.5f},{0.5f,0.5f,0.0f},{-0.5f,0.5f,0.0f}},
         positive_slope, {{0,0},{2,0},{2,1},{0,1}});
    face({{0.5f,0.0f,-0.5f},{-0.5f,0.0f,-0.5f},{-0.5f,0.5f,0.0f},{0.5f,0.5f,0.0f}},
         negative_slope, {{0,0},{2,0},{2,1},{0,1}});
    face({{-0.5f,0.0f,-0.5f},{-0.5f,0.0f,0.5f},{-0.5f,0.5f,0.0f}},
         {-1,0,0}, {{0,0},{1,0},{0.5f,1}});
    face({{0.5f,0.0f,0.5f},{0.5f,0.0f,-0.5f},{0.5f,0.5f,0.0f}},
         {1,0,0}, {{0,0},{1,0},{0.5f,1}});
    face({{-0.5f,0.0f,-0.5f},{0.5f,0.0f,-0.5f},{0.5f,0.0f,0.5f},{-0.5f,0.0f,0.5f}},
         {0,-1,0}, {{0,0},{1,0},{1,1},{0,1}});
    calculate_tangents(data);
    return data;
}

MeshData make_grid_mesh(int columns, int rows, float width, float depth, float uv_scale) {
    columns = std::max(columns, 2);
    rows = std::max(rows, 2);
    MeshData data;
    data.vertices.reserve(static_cast<std::size_t>(columns) * rows);
    data.indices.reserve(static_cast<std::size_t>(columns - 1) * (rows - 1) * 6u);
    for (int z = 0; z < rows; ++z) {
        const float vz = static_cast<float>(z) / static_cast<float>(rows - 1);
        for (int x = 0; x < columns; ++x) {
            const float vx = static_cast<float>(x) / static_cast<float>(columns - 1);
            data.vertices.push_back({
                {(vx - 0.5f) * width, 0.0f, (vz - 0.5f) * depth},
                {0.0f, 1.0f, 0.0f},
                {vx * uv_scale, vz * uv_scale},
                {1.0f, 0.0f, 0.0f, -1.0f}
            });
        }
    }
    for (int z = 0; z < rows - 1; ++z) {
        for (int x = 0; x < columns - 1; ++x) {
            const auto a = static_cast<std::uint32_t>(z * columns + x);
            const auto b = a + 1u;
            const auto c = a + static_cast<std::uint32_t>(columns);
            const auto d = c + 1u;
            data.indices.insert(data.indices.end(), {a, d, b, a, c, d});
        }
    }
    return data;
}

void calculate_tangents(MeshData& data) {
    std::vector<math::Vec3> tan1(data.vertices.size());
    std::vector<math::Vec3> tan2(data.vertices.size());
    for(std::size_t i=0;i+2<data.indices.size();i+=3){
        const auto i0=data.indices[i],i1=data.indices[i+1],i2=data.indices[i+2];
        const auto& v0=data.vertices[i0];const auto& v1=data.vertices[i1];const auto& v2=data.vertices[i2];
        const math::Vec3 e1=v1.position-v0.position,e2=v2.position-v0.position;
        const math::Vec2 d1=v1.uv-v0.uv,d2=v2.uv-v0.uv;
        const float det=d1.x*d2.y-d1.y*d2.x;
        if(std::abs(det)<1e-8f) continue;
        const float r=1.0f/det;
        const math::Vec3 sdir=(e1*d2.y-e2*d1.y)*r;
        const math::Vec3 tdir=(e2*d1.x-e1*d2.x)*r;
        tan1[i0]+=sdir;tan1[i1]+=sdir;tan1[i2]+=sdir;tan2[i0]+=tdir;tan2[i1]+=tdir;tan2[i2]+=tdir;
    }
    for(std::size_t i=0;i<data.vertices.size();++i){
        const math::Vec3 n=math::normalize(data.vertices[i].normal);
        math::Vec3 t=tan1[i]-n*math::dot(n,tan1[i]);
        if(math::length(t)<1e-6f) {
            const math::Vec3 axis = std::abs(n.y) < 0.95f ? math::Vec3{0,1,0} : math::Vec3{1,0,0};
            t = math::normalize(math::cross(axis,n));
        } else {
            t=math::normalize(t);
        }
        const float handedness=math::dot(math::cross(n,t),tan2[i])<0.0f?-1.0f:1.0f;
        data.vertices[i].tangent={t.x,t.y,t.z,handedness};
    }
}

} // namespace epoch::render::gl
