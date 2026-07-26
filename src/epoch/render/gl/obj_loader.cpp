#include "epoch/render/gl/mesh.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace epoch::render::gl {
namespace {
struct Key {
    int p{};
    int t{};
    int n{};
    bool operator==(const Key&) const = default;
};

struct KeyHash {
    std::size_t operator()(const Key& key) const noexcept {
        return (static_cast<std::size_t>(key.p) * 73856093u)
             ^ (static_cast<std::size_t>(key.t) * 19349663u)
             ^ (static_cast<std::size_t>(key.n) * 83492791u);
    }
};

Key parse_key(const std::string& text) {
    Key key{};
    char slash{};
    std::stringstream stream{text};
    stream >> key.p;
    if (stream.peek() == '/') {
        stream >> slash;
        if (stream.peek() != '/') stream >> key.t;
        if (stream.peek() == '/') {
            stream >> slash;
            stream >> key.n;
        }
    }
    return key;
}

int resolve_index(int index, std::size_t size) noexcept {
    return index > 0 ? index - 1 : index < 0 ? static_cast<int>(size) + index : -1;
}

void append_oriented_triangle(MeshData& data, std::uint32_t a, std::uint32_t b, std::uint32_t c) {
    const Vertex& va = data.vertices[a];
    const Vertex& vb = data.vertices[b];
    const Vertex& vc = data.vertices[c];

    const math::Vec3 geometric = math::cross(vb.position - va.position, vc.position - va.position);
    const math::Vec3 authored = va.normal + vb.normal + vc.normal;

    // Some public OBJ generators export valid normals with the triangle winding reversed.
    // Normalize every triangle against its authored vertex normals before uploading. This
    // keeps back-face culling correct without making individual models double-sided.
    if (math::length(geometric) > 1.0e-8f
        && math::length(authored) > 1.0e-6f
        && math::dot(geometric, authored) < 0.0f) {
        std::swap(b, c);
    }

    data.indices.push_back(a);
    data.indices.push_back(b);
    data.indices.push_back(c);
}
}

MeshData load_obj(const std::filesystem::path& path) {
    std::ifstream file{path};
    if (!file) throw std::runtime_error("Cannot open OBJ: " + path.string());

    std::vector<math::Vec3> positions;
    std::vector<math::Vec4> colors;
    std::vector<math::Vec3> normals;
    std::vector<math::Vec2> uvs;
    MeshData data;
    std::unordered_map<Key, std::uint32_t, KeyHash> cache;

    const auto vertex_for = [&](Key key) -> std::uint32_t {
        if (const auto it = cache.find(key); it != cache.end()) return it->second;

        const int position_index = resolve_index(key.p, positions.size());
        const int texture_index = resolve_index(key.t, uvs.size());
        const int normal_index = resolve_index(key.n, normals.size());
        if (position_index < 0 || position_index >= static_cast<int>(positions.size())) {
            throw std::runtime_error("OBJ position index out of range: " + path.string());
        }

        Vertex vertex{};
        vertex.position = positions[static_cast<std::size_t>(position_index)];
        if (static_cast<std::size_t>(position_index) < colors.size())
            vertex.color = colors[static_cast<std::size_t>(position_index)];
        if (texture_index >= 0 && texture_index < static_cast<int>(uvs.size())) {
            vertex.uv = uvs[static_cast<std::size_t>(texture_index)];
        }
        if (normal_index >= 0 && normal_index < static_cast<int>(normals.size())) {
            vertex.normal = normals[static_cast<std::size_t>(normal_index)];
        }

        const auto index = static_cast<std::uint32_t>(data.vertices.size());
        data.vertices.push_back(vertex);
        cache.emplace(key, index);
        return index;
    };

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream stream{line};
        std::string operation;
        stream >> operation;

        if (operation == "v") {
            math::Vec3 value{};
            stream >> value.x >> value.y >> value.z;
            positions.push_back(value);
            math::Vec4 color{1.0f, 1.0f, 1.0f, 1.0f};
            if (stream >> color.x >> color.y >> color.z) color.w = 1.0f;
            colors.push_back(color);
        } else if (operation == "vt") {
            math::Vec2 value{};
            stream >> value.x >> value.y;
            // OpenGL image uploads in this project use a bottom-left UV origin.
            // Preserve authored OBJ UVs; WIC texture loading performs the required flip.
            uvs.push_back(value);
        } else if (operation == "vn") {
            math::Vec3 value{};
            stream >> value.x >> value.y >> value.z;
            normals.push_back(math::normalize(value));
        } else if (operation == "f") {
            std::vector<std::uint32_t> polygon;
            std::string item;
            while (stream >> item) polygon.push_back(vertex_for(parse_key(item)));
            for (std::size_t i = 1; i + 1 < polygon.size(); ++i) {
                append_oriented_triangle(data, polygon[0], polygon[i], polygon[i + 1]);
            }
        }
    }

    if (data.vertices.empty() || data.indices.empty()) {
        throw std::runtime_error("OBJ contains no renderable triangles: " + path.string());
    }

    bool missing_normals = false;
    for (const auto& vertex : data.vertices) {
        if (math::length(vertex.normal) < 0.5f) {
            missing_normals = true;
            break;
        }
    }

    if (missing_normals) {
        for (auto& vertex : data.vertices) vertex.normal = {};
        for (std::size_t i = 0; i + 2 < data.indices.size(); i += 3) {
            auto& a = data.vertices[data.indices[i]];
            auto& b = data.vertices[data.indices[i + 1]];
            auto& c = data.vertices[data.indices[i + 2]];
            const auto normal = math::normalize(math::cross(b.position - a.position, c.position - a.position));
            a.normal += normal;
            b.normal += normal;
            c.normal += normal;
        }
        for (auto& vertex : data.vertices) vertex.normal = math::normalize(vertex.normal);
    }

    calculate_tangents(data);
    return data;
}

} // namespace epoch::render::gl
