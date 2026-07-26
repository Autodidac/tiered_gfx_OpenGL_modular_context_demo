#include "epoch/render/techniques/cloth_simulation.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <utility>

#include "epoch/render/techniques/shadow_mapping.hpp"

namespace epoch::render::techniques {
namespace {
constexpr float fixed_step = 1.0f / 120.0f;
constexpr float damping = 0.996f;
constexpr int constraint_iterations = 6;

template <typename Cloth>
[[nodiscard]] std::size_t index_of(const Cloth& cloth,
                                   std::uint32_t x, std::uint32_t y) noexcept {
    return static_cast<std::size_t>(y) * cloth.columns + x;
}

template <typename Particle>
void satisfy(Particle& a,
             Particle& b,
             float rest_length,
             float stiffness = 1.0f) noexcept {
    math::Vec3 delta = b.position - a.position;
    const float distance = math::length(delta);
    if (distance <= 1.0e-6f) return;
    const math::Vec3 correction = delta * ((distance - rest_length) / distance * stiffness);
    if (!a.pinned && !b.pinned) {
        a.position += correction * 0.5f;
        b.position -= correction * 0.5f;
    } else if (!a.pinned) {
        a.position += correction;
    } else if (!b.pinned) {
        b.position -= correction;
    }
}
}

ClothSimulationTechnique::ClothSimulationTechnique(
    const std::filesystem::path& root, ResourceSpine& resources)
    : shader_{root / "cloth/cloth.vert", root / "cloth/cloth.frag"},
      shadow_shader_{root / "cloth/cloth_shadow.vert", root / "cloth/cloth_shadow.frag"},
      fallback_albedo_{resources.load_texture(
          "default_pack/textures/defaults/white.png", gl::ColorSpace::srgb)},
      fallback_normal_{resources.load_texture(
          "curated/materials/woven_blue_fabric/normal.png", gl::ColorSpace::linear)},
      fallback_orm_{resources.load_texture(
          "curated/materials/woven_blue_fabric/orm.png", gl::ColorSpace::linear)} {}

ClothSimulationTechnique::~ClothSimulationTechnique() {
    for (auto& cloth : cloths_) destroy(cloth);
}

void ClothSimulationTechnique::destroy(RuntimeCloth& runtime) noexcept {
    if (runtime.ebo) gl::DeleteBuffers(1, &runtime.ebo);
    if (runtime.vbo) gl::DeleteBuffers(1, &runtime.vbo);
    if (runtime.vao) gl::DeleteVertexArrays(1, &runtime.vao);
    runtime.ebo = runtime.vbo = runtime.vao = 0;
}

void ClothSimulationTechnique::initialize(RuntimeCloth& runtime, const scene::ClothObject& descriptor) {
    runtime.columns = std::max<std::uint32_t>(descriptor.columns, 4u);
    runtime.rows = std::max<std::uint32_t>(descriptor.rows, 4u);
    runtime.rest_x = descriptor.width / static_cast<float>(runtime.columns - 1u);
    runtime.rest_y = descriptor.height / static_cast<float>(runtime.rows - 1u);
    runtime.descriptor_origin = descriptor.origin;
    runtime.descriptor_right_axis = descriptor.right_axis;
    runtime.descriptor_down_axis = descriptor.down_axis;
    runtime.descriptor_width = descriptor.width;
    runtime.descriptor_height = descriptor.height;
    const math::Vec3 right = math::normalize(descriptor.right_axis);
    const math::Vec3 down = math::normalize(descriptor.down_axis);
    math::Vec3 surface_normal = math::normalize(math::cross(right, down));
    if (math::length(surface_normal) < 0.5f) surface_normal = {0.0f, 0.0f, -1.0f};

    const std::size_t vertex_count = static_cast<std::size_t>(runtime.columns) * runtime.rows;
    runtime.particles.resize(vertex_count);
    runtime.vertices.resize(vertex_count);
    runtime.indices.clear();
    runtime.indices.reserve(static_cast<std::size_t>(runtime.columns - 1u) * (runtime.rows - 1u) * 6u);

    for (std::uint32_t y = 0; y < runtime.rows; ++y) {
        const float fy = static_cast<float>(y) / static_cast<float>(runtime.rows - 1u);
        for (std::uint32_t x = 0; x < runtime.columns; ++x) {
            const float fx = static_cast<float>(x) / static_cast<float>(runtime.columns - 1u);
            const float billow_shape = descriptor.pin_mode == scene::ClothPinMode::left_edge
                ? std::sin(fx * 3.14159265f * 0.84f) * (0.72f + 0.28f * std::sin(fy * 3.14159265f))
                : std::sin(fx * 3.14159265f) * std::sin(fy * 3.14159265f);
            const math::Vec3 position = descriptor.origin + right * (descriptor.width * fx)
                + down * (descriptor.height * fy)
                + surface_normal * (descriptor.initial_billow * billow_shape);
            bool pinned = false;
            switch (descriptor.pin_mode) {
            case scene::ClothPinMode::top_edge: pinned = y == 0; break;
            case scene::ClothPinMode::top_corners: pinned = y == 0 && (x == 0 || x + 1u == runtime.columns); break;
            case scene::ClothPinMode::left_edge: pinned = x == 0; break;
            }
            const math::Vec3 previous = pinned ? position : position - surface_normal * (0.006f * billow_shape);
            runtime.particles[index_of(runtime, x, y)] = {position, previous, pinned};
        }
    }

    for (std::uint32_t y = 0; y + 1u < runtime.rows; ++y) {
        for (std::uint32_t x = 0; x + 1u < runtime.columns; ++x) {
            const auto a = static_cast<std::uint32_t>(index_of(runtime, x, y));
            const auto b = static_cast<std::uint32_t>(index_of(runtime, x + 1u, y));
            const auto c = static_cast<std::uint32_t>(index_of(runtime, x, y + 1u));
            const auto d = static_cast<std::uint32_t>(index_of(runtime, x + 1u, y + 1u));
            runtime.indices.insert(runtime.indices.end(), {a, c, b, b, c, d});
        }
    }

    gl::GenVertexArrays(1, &runtime.vao);
    gl::GenBuffers(1, &runtime.vbo);
    gl::GenBuffers(1, &runtime.ebo);
    gl::BindVertexArray(runtime.vao);
    gl::BindBuffer(gl::array_buffer, runtime.vbo);
    gl::BufferData(gl::array_buffer,
        static_cast<gl::GLsizeiptr>(runtime.vertices.size() * sizeof(gl::Vertex)),
        nullptr, gl::dynamic_draw);
    gl::BindBuffer(gl::element_array_buffer, runtime.ebo);
    gl::BufferData(gl::element_array_buffer,
        static_cast<gl::GLsizeiptr>(runtime.indices.size() * sizeof(std::uint32_t)),
        runtime.indices.data(), gl::static_draw);
    constexpr GLsizei stride = sizeof(gl::Vertex);
    gl::EnableVertexAttribArray(0);
    gl::VertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride,
        reinterpret_cast<void*>(offsetof(gl::Vertex, position)));
    gl::EnableVertexAttribArray(1);
    gl::VertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride,
        reinterpret_cast<void*>(offsetof(gl::Vertex, normal)));
    gl::EnableVertexAttribArray(2);
    gl::VertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride,
        reinterpret_cast<void*>(offsetof(gl::Vertex, uv)));
    gl::EnableVertexAttribArray(3);
    gl::VertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, stride,
        reinterpret_cast<void*>(offsetof(gl::Vertex, tangent)));
    gl::BindVertexArray(0);
    upload(runtime, descriptor);
}

void ClothSimulationTechnique::synchronize(const scene::SceneSpine& scene) {
    if (cloths_.size() != scene.cloth_objects.size()) {
        for (auto& cloth : cloths_) destroy(cloth);
        cloths_.clear();
        cloths_.reserve(scene.cloth_objects.size());
        for (std::size_t index = 0; index < scene.cloth_objects.size(); ++index) {
            RuntimeCloth runtime;
            runtime.descriptor_index = index;
            initialize(runtime, scene.cloth_objects[index]);
            cloths_.push_back(std::move(runtime));
        }
        return;
    }

    constexpr float epsilon = 1.0e-4f;
    for (std::size_t index = 0; index < cloths_.size(); ++index) {
        auto& runtime = cloths_[index];
        const auto& descriptor = scene.cloth_objects[index];
        const bool dimensions_changed = std::abs(runtime.descriptor_width - descriptor.width) > epsilon
            || std::abs(runtime.descriptor_height - descriptor.height) > epsilon;
        const bool axes_changed = math::length(runtime.descriptor_right_axis - descriptor.right_axis) > epsilon
            || math::length(runtime.descriptor_down_axis - descriptor.down_axis) > epsilon;
        const bool topology_changed = runtime.columns != std::max<std::uint32_t>(descriptor.columns, 4u)
            || runtime.rows != std::max<std::uint32_t>(descriptor.rows, 4u);
        if (dimensions_changed || axes_changed || topology_changed) {
            destroy(runtime);
            runtime = RuntimeCloth{};
            runtime.descriptor_index = index;
            initialize(runtime, descriptor);
            continue;
        }

        const math::Vec3 translation = descriptor.origin - runtime.descriptor_origin;
        if (math::length(translation) > epsilon) {
            for (auto& particle : runtime.particles) {
                particle.position += translation;
                particle.previous += translation;
            }
            runtime.descriptor_origin = descriptor.origin;
            upload(runtime, descriptor);
        }
    }
}

void ClothSimulationTechnique::simulate(
    RuntimeCloth& cloth, const scene::ClothObject& descriptor,
    float delta_seconds, float elapsed_seconds, float wind_strength) {
    cloth.accumulator = std::min(cloth.accumulator + delta_seconds, fixed_step * 5.0f);
    const math::Vec3 right = math::normalize(descriptor.right_axis);
    const math::Vec3 down = math::normalize(descriptor.down_axis);
    math::Vec3 normal = math::normalize(math::cross(right, down));
    if (math::length(normal) < 0.5f) normal = {0.0f, 0.0f, -1.0f};

    while (cloth.accumulator >= fixed_step) {
        const float gust = 0.72f
            + std::sin(elapsed_seconds * 1.35f + static_cast<float>(cloth.descriptor_index)) * 0.24f
            + std::sin(elapsed_seconds * 0.43f + 2.3f) * 0.14f;
        const float wind_pressure = descriptor.wind_response * wind_strength * (12.0f + gust * 18.0f);
        const float step_squared = fixed_step * fixed_step;
        for (std::uint32_t y = 0; y < cloth.rows; ++y) {
            const float fy = static_cast<float>(y) / static_cast<float>(cloth.rows - 1u);
            for (std::uint32_t x = 0; x < cloth.columns; ++x) {
                const float fx = static_cast<float>(x) / static_cast<float>(cloth.columns - 1u);
                auto& particle = cloth.particles[index_of(cloth, x, y)];
                if (particle.pinned) continue;

                const float travelling = std::sin(elapsed_seconds * 3.15f - fx * 9.4f + fy * 1.7f
                    + static_cast<float>(cloth.descriptor_index) * 0.91f);
                const float flutter = std::sin(elapsed_seconds * 7.4f - fx * 17.0f - fy * 3.2f) * 0.32f;
                const float free_edge = descriptor.pin_mode == scene::ClothPinMode::left_edge
                    ? (0.15f + 0.85f * fx * fx)
                    : (0.35f + 0.65f * std::sin(fx * 3.14159265f));
                const math::Vec3 acceleration =
                    math::Vec3{0.0f, -9.81f * descriptor.gravity_scale, 0.0f}
                    + normal * wind_pressure * free_edge * (0.72f + travelling * 0.30f + flutter * free_edge)
                    + right * (travelling * 2.7f + flutter * 1.5f) * descriptor.wind_response * wind_strength * free_edge
                    - down * std::max(0.0f, travelling) * 0.72f * descriptor.wind_response * wind_strength * free_edge;

                const math::Vec3 current = particle.position;
                const math::Vec3 velocity = (particle.position - particle.previous) * damping;
                particle.position += velocity + acceleration * step_squared;
                particle.previous = current;
            }
        }

        const float diagonal = std::sqrt(cloth.rest_x * cloth.rest_x + cloth.rest_y * cloth.rest_y);
        for (int iteration = 0; iteration < constraint_iterations; ++iteration) {
            for (std::uint32_t y = 0; y < cloth.rows; ++y) {
                for (std::uint32_t x = 0; x < cloth.columns; ++x) {
                    auto& particle = cloth.particles[index_of(cloth, x, y)];
                    if (x + 1u < cloth.columns)
                        satisfy(particle, cloth.particles[index_of(cloth, x + 1u, y)], cloth.rest_x);
                    if (y + 1u < cloth.rows)
                        satisfy(particle, cloth.particles[index_of(cloth, x, y + 1u)], cloth.rest_y);
                    if (x + 1u < cloth.columns && y + 1u < cloth.rows)
                        satisfy(particle, cloth.particles[index_of(cloth, x + 1u, y + 1u)], diagonal, 0.88f);
                    if (x > 0u && y + 1u < cloth.rows)
                        satisfy(particle, cloth.particles[index_of(cloth, x - 1u, y + 1u)], diagonal, 0.88f);
                    if (x + 2u < cloth.columns)
                        satisfy(particle, cloth.particles[index_of(cloth, x + 2u, y)], cloth.rest_x * 2.0f, 0.18f);
                    if (y + 2u < cloth.rows)
                        satisfy(particle, cloth.particles[index_of(cloth, x, y + 2u)], cloth.rest_y * 2.0f, 0.18f);
                    if (!particle.pinned && particle.position.y < 0.12f) particle.position.y = 0.12f;
                }
            }
        }
        cloth.accumulator -= fixed_step;
    }

    upload(cloth, descriptor);
}

void ClothSimulationTechnique::upload(RuntimeCloth& cloth, const scene::ClothObject& descriptor) {
    for (auto& vertex : cloth.vertices) vertex.normal = {};
    for (std::size_t i = 0; i + 2u < cloth.indices.size(); i += 3u) {
        const auto ia = cloth.indices[i];
        const auto ib = cloth.indices[i + 1u];
        const auto ic = cloth.indices[i + 2u];
        const math::Vec3 a = cloth.particles[ia].position;
        const math::Vec3 b = cloth.particles[ib].position;
        const math::Vec3 c = cloth.particles[ic].position;
        const math::Vec3 face = math::cross(b - a, c - a);
        cloth.vertices[ia].normal += face;
        cloth.vertices[ib].normal += face;
        cloth.vertices[ic].normal += face;
    }
    for (std::uint32_t y = 0; y < cloth.rows; ++y) {
        for (std::uint32_t x = 0; x < cloth.columns; ++x) {
            const std::size_t index = index_of(cloth, x, y);
            const math::Vec3 normal = math::normalize(cloth.vertices[index].normal);
            math::Vec3 tangent{};
            if (x + 1u < cloth.columns)
                tangent = cloth.particles[index_of(cloth, x + 1u, y)].position - cloth.particles[index].position;
            else
                tangent = cloth.particles[index].position - cloth.particles[index_of(cloth, x - 1u, y)].position;
            tangent = math::normalize(tangent - normal * math::dot(normal, tangent));
            const float u = static_cast<float>(x) / static_cast<float>(cloth.columns - 1u);
            float v = static_cast<float>(y) / static_cast<float>(cloth.rows - 1u);
            if (descriptor.name.find("flag cloth") != std::string::npos)
                v = 1.0f - v;
            cloth.vertices[index] = {
                cloth.particles[index].position,
                normal,
                {u * descriptor.uv_scale.x,
                 v * descriptor.uv_scale.y},
                {tangent.x, tangent.y, tangent.z, 1.0f}
            };
        }
    }
    gl::BindBuffer(gl::array_buffer, cloth.vbo);
    gl::BufferSubData(gl::array_buffer, 0,
        static_cast<gl::GLsizeiptr>(cloth.vertices.size() * sizeof(gl::Vertex)),
        cloth.vertices.data());
}

void ClothSimulationTechnique::update(
    const scene::SceneSpine& scene, float delta_seconds, float elapsed_seconds,
    const context::RuntimeControls& controls) {
    synchronize(scene);
    if (!controls.cloth_simulation) return;
    const float dt = std::clamp(delta_seconds * controls.animation_speed, 0.0f, 1.0f / 20.0f);
    for (auto& runtime : cloths_) {
        const auto& descriptor = scene.cloth_objects[runtime.descriptor_index];
        if (!descriptor.visible) continue;
        simulate(runtime, descriptor, dt, elapsed_seconds, controls.cloth_wind_strength);
    }
}

void ClothSimulationTechnique::render_shadow(const TechniqueContext& frame, const scene::SceneSpine& scene) const {
    if (!frame.controls.cloth_simulation || !frame.controls.shadows
        || !frame.controls.directional_light || scene.sun.direction.y >= -0.03f) return;
    gl::Disable(GL_CULL_FACE);
    gl::Enable(GL_POLYGON_OFFSET_FILL);
    gl::PolygonOffset(1.5f, 3.0f);
    shadow_shader_.bind();
    shadow_shader_.set("uLightViewProjection", frame.light_view_projection);
    for (const auto& runtime : cloths_) {
        const auto& descriptor = scene.cloth_objects[runtime.descriptor_index];
        if (!descriptor.visible) continue;
        gl::BindVertexArray(runtime.vao);
        gl::DrawElements(GL_TRIANGLES, static_cast<GLsizei>(runtime.indices.size()), GL_UNSIGNED_INT, nullptr);
    }
    gl::Disable(GL_POLYGON_OFFSET_FILL);
    gl::Enable(GL_CULL_FACE);
    gl::CullFace(GL_BACK);
}

void ClothSimulationTechnique::render(
    const TechniqueContext& frame, const scene::SceneSpine& scene,
    const ResourceSpine& resources, const ShadowMappingTechnique& shadows) const {
    if (!frame.controls.cloth_simulation) return;
    gl::Disable(GL_CULL_FACE);
    shader_.bind();
    shader_.set("uViewProjection", frame.view_projection);
    shader_.set("uLightViewProjection", frame.light_view_projection);
    shader_.set("uCameraPosition", frame.camera_position);
    shader_.set("uSunDirection", scene.sun.direction);
    shader_.set("uSunColor", scene.sun.color * frame.controls.sun_intensity);
    shader_.set("uEnvironmentStrength", frame.controls.environment_strength);
    shader_.set("uFogEnabled", frame.controls.fog ? 1 : 0);
    shader_.set("uFogDensity", frame.controls.fog_density);
    shader_.set("uFogHeightFalloff", frame.controls.fog_height_falloff);
    shader_.set("uShadowsEnabled", frame.controls.shadows ? 1 : 0);
    shader_.set("uAlbedo", 0);
    shader_.set("uNormal", 1);
    shader_.set("uOrm", 2);
    shader_.set("uShadowMap", 3);
    shader_.set("uEnvironment", 4);
    shadows.bind_depth(3);
    resources.cubemap(scene.environment).bind(4);

    for (const auto& runtime : cloths_) {
        const auto& descriptor = scene.cloth_objects[runtime.descriptor_index];
        if (!descriptor.visible) continue;
        const TextureHandle albedo = descriptor.albedo ? descriptor.albedo : fallback_albedo_;
        const TextureHandle normal = descriptor.normal ? descriptor.normal : fallback_normal_;
        const TextureHandle orm = descriptor.orm ? descriptor.orm : fallback_orm_;
        resources.texture(albedo).bind(0);
        resources.texture(normal).bind(1);
        resources.texture(orm).bind(2);
        shader_.set("uTint", descriptor.tint);
        gl::BindVertexArray(runtime.vao);
        gl::DrawElements(GL_TRIANGLES, static_cast<GLsizei>(runtime.indices.size()), GL_UNSIGNED_INT, nullptr);
    }
    gl::Enable(GL_CULL_FACE);
}

} // namespace epoch::render::techniques
