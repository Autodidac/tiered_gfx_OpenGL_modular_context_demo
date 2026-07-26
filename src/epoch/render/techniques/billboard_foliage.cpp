#include "epoch/render/techniques/billboard_foliage.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace epoch::render::techniques {
namespace {
struct QuadVertex {
    math::Vec2 local{};
    math::Vec2 uv{};
};

[[nodiscard]] float smoothstep(float edge0, float edge1, float value) noexcept {
    if (edge0 == edge1) return value < edge0 ? 0.0f : 1.0f;
    const float t = std::clamp((value - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

[[nodiscard]] float sample_red(const gl::Image& image, float u, float v) noexcept {
    if (image.width <= 0 || image.height <= 0 || image.channels <= 0 || image.pixels.empty()) return 0.5f;
    u = std::clamp(u, 0.0f, 1.0f);
    v = std::clamp(v, 0.0f, 1.0f);
    const float x = u * static_cast<float>(image.width - 1);
    const float y = v * static_cast<float>(image.height - 1);
    const int x0 = static_cast<int>(std::floor(x));
    const int y0 = static_cast<int>(std::floor(y));
    const int x1 = std::min(x0 + 1, image.width - 1);
    const int y1 = std::min(y0 + 1, image.height - 1);
    const float tx = x - static_cast<float>(x0);
    const float ty = y - static_cast<float>(y0);
    const auto texel = [&](int px, int py) noexcept {
        const std::size_t offset = (static_cast<std::size_t>(py) * image.width + px)
            * static_cast<std::size_t>(image.channels);
        return static_cast<float>(image.pixels[offset]) / 255.0f;
    };
    const float a = texel(x0, y0) + (texel(x1, y0) - texel(x0, y0)) * tx;
    const float b = texel(x0, y1) + (texel(x1, y1) - texel(x0, y1)) * tx;
    return a + (b - a) * ty;
}

[[nodiscard]] bool mask_allows(const scene::FoliageRegion& region, float u, float v) noexcept {
    const auto resolution = static_cast<int>(scene::foliage_mask_resolution);
    const int x = std::clamp(static_cast<int>(u * static_cast<float>(resolution)), 0, resolution - 1);
    const int y = std::clamp(static_cast<int>(v * static_cast<float>(resolution)), 0, resolution - 1);
    return region.placement_mask[static_cast<std::size_t>(y) * scene::foliage_mask_resolution
        + static_cast<std::size_t>(x)] >= 128u;
}
}

BillboardFoliageTechnique::BillboardFoliageTechnique(
    const std::filesystem::path& root, ResourceSpine& resources, MeshHandle)
    : shader_{root / "billboard/billboard.vert", root / "billboard/billboard.frag"},
      albedo_{resources.load_texture("curated/foliage/grass_clump_albedo.png", gl::ColorSpace::srgb)},
      opacity_{resources.load_texture("curated/foliage/grass_clump_opacity.png", gl::ColorSpace::linear)},
      terrain_height_image_{gl::load_image(resources.asset_root() / "default_pack/maps/starter_island_height.png", true)},
      detail_height_image_{gl::load_image(resources.asset_root() / "default_pack/textures/materials/rock/height.png", true)} {
    // Golden-angle distribution keeps every density prefix spread across the
    // complete paintable region instead of collapsing lower densities to a corner.
    constexpr int maximum_instances = 3072;
    constexpr float golden_angle = 2.39996322972865332f;
    templates_.reserve(maximum_instances);
    for (int index = 0; index < maximum_instances; ++index) {
        const float sequence = static_cast<float>(index) + 0.5f;
        const float angle = sequence * golden_angle;
        const float radial = std::sqrt(sequence / static_cast<float>(maximum_instances));
        const float ripple = std::sin(angle * 2.7f) * 0.040f + std::sin(angle * 0.41f) * 0.022f;
        const float local_x = std::clamp(std::cos(angle) * radial + ripple, -0.995f, 0.995f);
        const float local_z = std::clamp(std::sin(angle) * radial + ripple * 0.65f, -0.995f, 0.995f);
        const float variation = 0.5f + 0.5f * std::sin(sequence * 12.9898f);
        templates_.push_back({
            {local_x, variation * 0.025f, local_z},
            {0.36f + variation * 0.30f, 0.62f + variation * 0.52f},
            angle + variation * 5.3f,
            0.72f + variation * 0.52f
        });
    }
    visible_instances_.reserve(maximum_instances);

    constexpr std::array<QuadVertex, 4> vertices{{
        {{-0.5f, 0.0f}, {0.0f, 0.0f}},
        {{ 0.5f, 0.0f}, {1.0f, 0.0f}},
        {{ 0.5f, 1.0f}, {1.0f, 1.0f}},
        {{-0.5f, 1.0f}, {0.0f, 1.0f}}
    }};
    constexpr std::array<std::uint32_t, 6> indices{0,1,2,0,2,3};

    gl::GenVertexArrays(1, &vao_);
    gl::GenBuffers(1, &vertex_buffer_);
    gl::GenBuffers(1, &index_buffer_);
    gl::GenBuffers(1, &instance_buffer_);
    gl::BindVertexArray(vao_);

    gl::BindBuffer(gl::array_buffer, vertex_buffer_);
    gl::BufferData(gl::array_buffer, static_cast<gl::GLsizeiptr>(sizeof(vertices)), vertices.data(), gl::static_draw);
    gl::EnableVertexAttribArray(0);
    gl::VertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(QuadVertex),
                            reinterpret_cast<void*>(offsetof(QuadVertex, local)));
    gl::EnableVertexAttribArray(1);
    gl::VertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(QuadVertex),
                            reinterpret_cast<void*>(offsetof(QuadVertex, uv)));

    gl::BindBuffer(gl::element_array_buffer, index_buffer_);
    gl::BufferData(gl::element_array_buffer, static_cast<gl::GLsizeiptr>(sizeof(indices)), indices.data(), gl::static_draw);

    gl::BindBuffer(gl::array_buffer, instance_buffer_);
    gl::BufferData(gl::array_buffer,
                   static_cast<gl::GLsizeiptr>(templates_.size() * sizeof(Instance)),
                   nullptr, gl::dynamic_draw);
    gl::EnableVertexAttribArray(2);
    gl::VertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Instance),
                            reinterpret_cast<void*>(offsetof(Instance, center)));
    gl::EnableVertexAttribArray(3);
    gl::VertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Instance),
                            reinterpret_cast<void*>(offsetof(Instance, size)));
    gl::EnableVertexAttribArray(4);
    gl::VertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, sizeof(Instance),
                            reinterpret_cast<void*>(offsetof(Instance, phase)));
    gl::EnableVertexAttribArray(5);
    gl::VertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, sizeof(Instance),
                            reinterpret_cast<void*>(offsetof(Instance, stiffness)));
    for (GLuint location = 2; location <= 5; ++location) gl::VertexAttribDivisor(location, 1);
    gl::BindVertexArray(0);
}

BillboardFoliageTechnique::~BillboardFoliageTechnique() {
    if (instance_buffer_) gl::DeleteBuffers(1, &instance_buffer_);
    if (index_buffer_) gl::DeleteBuffers(1, &index_buffer_);
    if (vertex_buffer_) gl::DeleteBuffers(1, &vertex_buffer_);
    if (vao_) gl::DeleteVertexArrays(1, &vao_);
}

float BillboardFoliageTechnique::terrain_height(math::Vec2 world_xz,
                                                 const scene::TerrainSurface& terrain,
                                                 bool detailed,
                                                 float tessellation_level) const noexcept {
    const float dx = world_xz.x - terrain.transform.position.x;
    const float dz = world_xz.y - terrain.transform.position.z;
    const float cosine = std::cos(terrain.transform.rotation.y);
    const float sine = std::sin(terrain.transform.rotation.y);
    const float local_x = (cosine * dx - sine * dz)
        / std::max(std::abs(terrain.transform.scale.x), 0.001f);
    const float local_z = (sine * dx + cosine * dz)
        / std::max(std::abs(terrain.transform.scale.z), 0.001f);
    if (std::abs(local_x) > 1.0f || std::abs(local_z) > 1.0f)
        return std::numeric_limits<float>::quiet_NaN();

    const float u = local_x * 0.5f + 0.5f;
    const float v = local_z * 0.5f + 0.5f;
    const math::Vec2 terrain_domain{(u - 0.5f) * 64.0f, (v - 0.5f) * 64.0f - 3.0f};
    float height = (sample_red(terrain_height_image_, u, v) - 0.42f) * terrain.height_scale;

    const float hall_x = std::abs(terrain_domain.x / 17.0f);
    const float hall_z = std::abs((terrain_domain.y + 14.0f) / 7.2f);
    const float hall_flat = 1.0f - smoothstep(0.88f, 1.08f, std::max(hall_x, hall_z));
    const float plaza_x = std::abs(terrain_domain.x / 19.0f);
    const float plaza_z = std::abs((terrain_domain.y - 3.0f) / 11.0f);
    const float plaza_flat = 1.0f - smoothstep(0.86f, 1.12f, std::max(plaza_x, plaza_z));
    height += (0.0f - height) * std::max(hall_flat, plaza_flat);

    const float pool_x = std::abs((terrain_domain.x - 16.0f) / 6.2f);
    const float pool_z = std::abs((terrain_domain.y - 5.0f) / 4.0f);
    const float pool_cut = 1.0f - smoothstep(0.82f, 1.08f, std::max(pool_x, pool_z));
    height += (-0.38f - height) * pool_cut;

    const float lake_x = terrain_domain.x / 25.0f;
    const float lake_z = (terrain_domain.y + 30.0f) / 8.0f;
    const float lake_cut = 1.0f - smoothstep(0.76f, 1.02f, std::sqrt(lake_x * lake_x + lake_z * lake_z));
    height += (-0.72f - height) * lake_cut;

    if (detailed) {
        const float protected_area = std::max(std::max(hall_flat, plaza_flat), std::max(pool_cut, lake_cut));
        const float detail_u = terrain_domain.x * 0.16f - std::floor(terrain_domain.x * 0.16f);
        const float detail_v = terrain_domain.y * 0.16f - std::floor(terrain_domain.y * 0.16f);
        const float detail = sample_red(detail_height_image_, detail_u, detail_v) - 0.5f;
        const float normalized_level = std::clamp((tessellation_level - 1.0f) / 31.0f, 0.0f, 1.0f);
        const float detail_gain = (0.16f + 0.82f * std::sqrt(normalized_level)) * (1.0f - protected_area);
        height += detail * detail_gain;
    }

    return terrain.transform.position.y + height * terrain.transform.scale.y;
}

bool BillboardFoliageTechnique::excluded_by_solid(math::Vec3 world_position,
                                                   const scene::SceneSpine& scene) const noexcept {
    for (const auto& object : scene.objects) {
        if (!object.visible || object.editor_deleted || object.editor_only || object.camera_attached) continue;
        const float half_x = std::abs(object.transform.scale.x);
        const float half_y = std::abs(object.transform.scale.y);
        const float half_z = std::abs(object.transform.scale.z);
        if (half_x < 0.06f || half_z < 0.06f) continue;

        // Ignore objects clearly above or below this patch of ground.
        if (world_position.y > object.transform.position.y + half_y + 0.35f
            || world_position.y < object.transform.position.y - half_y - 0.80f) continue;

        const float dx = world_position.x - object.transform.position.x;
        const float dz = world_position.z - object.transform.position.z;
        const float cosine = std::cos(object.transform.rotation.y);
        const float sine = std::sin(object.transform.rotation.y);
        const float local_x = cosine * dx - sine * dz;
        const float local_z = sine * dx + cosine * dz;

        // Shrink the solid footprint slightly. Grass may creep under the outer edge,
        // but can never be painted into the object's interior.
        const float creep = std::clamp(std::min(half_x, half_z) * 0.12f, 0.08f, 0.32f);
        const float blocked_x = std::max(0.0f, half_x - creep);
        const float blocked_z = std::max(0.0f, half_z - creep);
        if (std::abs(local_x) <= blocked_x && std::abs(local_z) <= blocked_z) return true;
    }
    return false;
}

void BillboardFoliageTechnique::render(
    const TechniqueContext& frame, const scene::SceneSpine& scene,
    const ResourceSpine& resources) const {
    if (!frame.controls.billboards || !scene.terrain.visible) return;

    gl::Disable(GL_CULL_FACE);
    gl::DepthMask(GL_TRUE);
    shader_.bind();
    shader_.set("uViewProjection", frame.view_projection);
    shader_.set("uCameraRight", frame.camera_right);
    shader_.set("uTime", frame.elapsed_seconds * frame.controls.animation_speed);
    shader_.set("uCrossAngle", 0.0f);
    shader_.set("uSunDirection", scene.sun.direction);
    shader_.set("uSunColor", scene.sun.color * frame.controls.sun_intensity);
    shader_.set("uAlbedo", 0);
    shader_.set("uOpacity", 1);
    resources.texture(albedo_).bind(0);
    resources.texture(opacity_).bind(1);
    gl::BindVertexArray(vao_);

    const float global_density = std::clamp(frame.controls.foliage_density, 0.25f, 2.0f);
    for (const auto& region : scene.foliage_regions) {
        if (!region.visible) continue;
        const float density = std::clamp(global_density * region.density, 0.0f, 4.0f);
        const auto requested = std::min(
            static_cast<std::size_t>(std::lround(1800.0f * density)), templates_.size());
        visible_instances_.clear();
        visible_instances_.reserve(requested);
        const math::Mat4 region_transform = region.transform.matrix();
        const float height_offset = region.transform.position.y + 1.0f;
        for (std::size_t index = 0; index < requested; ++index) {
            const auto& source = templates_[index];
            const float mask_u = source.center.x * 0.5f + 0.5f;
            const float mask_v = source.center.z * 0.5f + 0.5f;
            if (!mask_allows(region, mask_u, mask_v)) continue;

            const math::Vec4 transformed = region_transform * math::Vec4{
                source.center.x, 0.0f, source.center.z, 1.0f};
            Instance placed = source;
            placed.center.x = transformed.x;
            placed.center.z = transformed.z;
            const float ground_height = terrain_height(
                {placed.center.x, placed.center.z}, scene.terrain, frame.controls.tessellation,
                frame.controls.tessellation_level);
            if (!std::isfinite(ground_height)) continue;
            placed.center.y = ground_height + height_offset + source.center.y - 0.015f;
            if (excluded_by_solid(placed.center, scene)) continue;
            visible_instances_.push_back(placed);
        }
        if (visible_instances_.empty()) continue;

        gl::BindBuffer(gl::array_buffer, instance_buffer_);
        gl::BufferData(gl::array_buffer,
            static_cast<gl::GLsizeiptr>(visible_instances_.size() * sizeof(Instance)),
            visible_instances_.data(), gl::dynamic_draw);
        shader_.set("uBladeScale", region.blade_scale);
        shader_.set("uSwayScale", region.sway);
        shader_.set("uCrossAngle", 0.0f);
        gl::DrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr,
                                  static_cast<GLsizei>(visible_instances_.size()));
        if (!frame.controls.tier0_mobile_profile) {
            shader_.set("uCrossAngle", 1.57079632679f);
            gl::DrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr,
                                      static_cast<GLsizei>(visible_instances_.size()));
        }
    }
    gl::Enable(GL_CULL_FACE);
}

} // namespace epoch::render::techniques
