#include "epoch/gui/gui_overlay.hpp"
#include "epoch/render/gl/gl_api.hpp"
#include "epoch/render/resource_spine.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <exception>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <limits>
#include <ranges>
#include <sstream>
#include <string>
#include <utility>
#include <system_error>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace epoch::gui {
namespace gl = epoch::render::gl;
namespace egl = epochengine::gui_lib;
namespace {
constexpr math::Vec4 panel_color{0.035f, 0.045f, 0.065f, 0.96f};
constexpr math::Vec4 title_color{0.055f, 0.085f, 0.13f, 0.99f};
constexpr math::Vec4 text_color{0.87f, 0.91f, 0.96f, 1.0f};
constexpr math::Vec4 muted_color{0.52f, 0.60f, 0.70f, 1.0f};
constexpr math::Vec4 accent_color{0.16f, 0.58f, 0.95f, 1.0f};
constexpr math::Vec4 control_color{0.095f, 0.12f, 0.17f, 1.0f};
constexpr math::Vec4 hover_color{0.14f, 0.18f, 0.25f, 1.0f};
constexpr math::Vec4 section_color{0.08f, 0.105f, 0.145f, 1.0f};
constexpr float base_glyph_width = 15.0f;
constexpr float base_glyph_height = 24.0f;
constexpr float base_glyph_advance = 14.0f;
constexpr float row_height = 34.0f;
constexpr float section_height = 28.0f;
constexpr float numeric_row_height = 30.0f;
constexpr std::size_t tuning_edit_base = 1000u;
constexpr std::size_t invalid_editor_object = static_cast<std::size_t>(-1);
constexpr std::size_t tab_count = 5;

constexpr std::array<std::string_view, tab_count> tab_names{
    "Lighting", "Tier 0", "Tier 1", "Tuning", "System"
};

[[nodiscard]] float tab_content_height(std::uint32_t tab) noexcept {
    switch (tab) {
    case 0: return row_height * 16.0f + section_height * 3.0f;
    case 1: return row_height * 15.0f + section_height * 4.0f;
    case 2: return row_height * 7.0f + section_height * 2.0f + 28.0f;
    case 3: return numeric_row_height * 26.0f + section_height * 5.0f;
    case 4: return row_height * 11.0f + section_height * 3.0f;
    default: return 0.0f;
    }
}

std::string_view scene_name(context::ScenePreset value) {
    switch (value) {
    case context::ScenePreset::material_lab: return "Natural daylight";
    case context::ScenePreset::lighting_lab: return "Warm dusk";
    case context::ScenePreset::asset_gallery: return "Night lighting";
    case context::ScenePreset::diagnostics: return "Mesh diagnostics";
    case context::ScenePreset::advanced_lab: return "Windy overcast";
    }
    return "Unknown";
}

std::string_view shading_name(context::ShadingMode value) {
    switch (value) {
    case context::ShadingMode::pbr: return "PBR";
    case context::ShadingMode::blinn_phong: return "Blinn-Phong";
    case context::ShadingMode::unlit: return "Unlit";
    case context::ShadingMode::normals: return "World normals";
    case context::ShadingMode::uv: return "UV coordinates";
    case context::ShadingMode::roughness: return "Roughness";
    case context::ShadingMode::metallic: return "Metallic";
    case context::ShadingMode::ambient_occlusion: return "Material AO";
    case context::ShadingMode::shadow_visibility: return "Shadow visibility";
    }
    return "Unknown";
}

[[nodiscard]] math::Vec4 with_alpha(math::Vec4 color, float alpha) noexcept {
    color.w *= alpha;
    return color;
}

[[nodiscard]] math::Vec3 degrees(math::Vec3 radians_value) noexcept {
    constexpr float factor = 57.29577951308232f;
    return radians_value * factor;
}

[[nodiscard]] math::Vec3 radians(math::Vec3 degrees_value) noexcept {
    return {math::radians(degrees_value.x), math::radians(degrees_value.y), math::radians(degrees_value.z)};
}

[[nodiscard]] math::Vec3 rotate_editor_offset(math::Vec3 value, math::Vec3 delta_radians) noexcept {
    const math::Mat4 rotation = math::rotate_y(delta_radians.y)
        * math::rotate_x(delta_radians.x)
        * math::rotate_z(delta_radians.z);
    const math::Vec4 rotated = rotation * math::Vec4{value.x, value.y, value.z, 0.0f};
    return {rotated.x, rotated.y, rotated.z};
}

[[nodiscard]] float safe_ratio(float next, float previous) noexcept {
    return std::abs(previous) > 0.0001f ? next / previous : 1.0f;
}

[[nodiscard]] render::scene::SpotLight& spotlight_at(
    render::scene::SceneSpine& scene, std::size_t index) noexcept {
    if (index == 0u) return scene.spotlight;
    if (index == 1u) return scene.secondary_spotlight;
    return scene.projector_spotlight;
}

[[nodiscard]] std::array<float, 4> editor_properties(
    const render::scene::SceneSpine&, const render::scene::RenderObject& object) noexcept {
    return object.editor_properties;
}

void set_editor_properties(render::scene::SceneSpine& scene,
                           render::scene::RenderObject& object,
                           const std::array<float, 4>& properties) noexcept {
    object.editor_properties = properties;
    using render::scene::EditorBindingKind;
    switch (object.editor_binding) {
    case EditorBindingKind::water_surface:
        if (object.editor_binding_index < scene.water_surfaces.size()) {
            auto& value = scene.water_surfaces[object.editor_binding_index];
            value.wave_amplitude = std::clamp(properties[0], 0.0f, 0.5f);
            value.wave_speed = std::clamp(properties[1], 0.0f, 4.0f);
            value.roughness = std::clamp(properties[2], 0.0f, 1.0f);
            value.opacity = std::clamp(properties[3], 0.0f, 1.0f);
            object.editor_properties = {value.wave_amplitude, value.wave_speed, value.roughness, value.opacity};
        }
        break;
    case EditorBindingKind::cloth_object:
        if (object.editor_binding_index < scene.cloth_objects.size()) {
            auto& value = scene.cloth_objects[object.editor_binding_index];
            value.wind_response = std::clamp(properties[0], 0.0f, 4.0f);
            value.gravity_scale = std::clamp(properties[1], 0.0f, 2.0f);
            value.initial_billow = std::clamp(properties[2], 0.0f, 1.0f);
            object.editor_properties = {value.wind_response, value.gravity_scale, value.initial_billow, 0.0f};
        }
        break;
    case EditorBindingKind::point_light:
        if (object.editor_binding_index < scene.point_light_count) {
            auto& value = scene.point_lights[object.editor_binding_index];
            value.radius = std::clamp(properties[0], 0.25f, 50.0f);
            const float remembered_intensity = std::clamp(properties[1], 0.0f, 80.0f);
            value.intensity = object.visible ? remembered_intensity : 0.0f;
            object.editor_properties = {value.radius, remembered_intensity, 0.0f, 0.0f};
        }
        break;
    case EditorBindingKind::spotlight: {
        auto& light = spotlight_at(scene, object.editor_binding_index);
        light.range = std::clamp(properties[0], 0.25f, 80.0f);
        light.intensity = std::clamp(properties[1], 0.0f, 100.0f);
        light.inner_cosine = std::clamp(properties[2], 0.0f, 1.0f);
        light.outer_cosine = std::clamp(properties[3], 0.0f, light.inner_cosine);
        light.enabled = object.visible;
        object.editor_properties = {light.range, light.intensity,
                                    light.inner_cosine, light.outer_cosine};
        break;
    }
    case EditorBindingKind::terrain_surface:
        scene.terrain.height_scale = std::clamp(properties[0], 0.0f, 24.0f);
        object.editor_properties = {scene.terrain.height_scale, 0.0f, 0.0f, 0.0f};
        break;
    case EditorBindingKind::foliage_region:
        if (object.editor_binding_index < scene.foliage_regions.size()) {
            auto& value = scene.foliage_regions[object.editor_binding_index];
            value.density = std::clamp(properties[0], 0.0f, 4.0f);
            value.blade_scale.x = std::clamp(properties[1], 0.0f, 4.0f);
            value.blade_scale.y = std::clamp(properties[2], 0.0f, 6.0f);
            value.sway = std::clamp(properties[3], 0.0f, 4.0f);
            object.editor_properties = {value.density, value.blade_scale.x, value.blade_scale.y, value.sway};
        }
        break;
    case EditorBindingKind::particle_emitter:
        if (object.editor_binding_index < scene.particle_emitters.size()) {
            auto& value = scene.particle_emitters[object.editor_binding_index];
            value.intensity = std::clamp(properties[0], 0.0f, 4.0f);
            value.size_scale = std::clamp(properties[1], 0.0f, 6.0f);
            object.editor_properties = {value.intensity, value.size_scale, 0.0f, 0.0f};
        }
        break;
    default:
        break;
    }
}

[[nodiscard]] render::MaterialHandle material_handle_by_name(
    const render::scene::SceneSpine& scene, std::string_view name) noexcept {
    if (name.empty()) return {};
    const auto found = std::ranges::find_if(scene.materials, [&](const auto& material) {
        return material.name == name;
    });
    if (found == scene.materials.end()) return {};
    return render::MaterialHandle{
        static_cast<std::uint32_t>(std::distance(scene.materials.begin(), found) + 1)};
}

[[nodiscard]] bool different(float lhs, float rhs) noexcept {
    return std::abs(lhs - rhs) > 0.00001f;
}

[[nodiscard]] bool replace_file_atomically(const std::filesystem::path& source,
                                            const std::filesystem::path& destination) noexcept {
#if defined(_WIN32)
    return MoveFileExW(source.c_str(), destination.c_str(),
                       MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != FALSE;
#else
    std::error_code error;
    std::filesystem::rename(source, destination, error);
    return !error;
#endif
}

void clamp_controls(context::RuntimeControls& controls) noexcept {
    controls.target_fps = std::clamp(controls.target_fps, 30.0f, 500.0f);
    controls.exposure = std::clamp(controls.exposure, 0.25f, 3.0f);
    controls.gamma = std::clamp(controls.gamma, 0.8f, 4.0f);
    controls.bloom_strength = std::clamp(controls.bloom_strength, 0.0f, 1.5f);
    controls.bloom_threshold = std::clamp(controls.bloom_threshold, 0.35f, 3.0f);
    controls.normal_strength = std::clamp(controls.normal_strength, 0.0f, 2.0f);
    controls.parallax_strength = std::clamp(controls.parallax_strength, 0.0f, 2.0f);
    controls.sun_intensity = std::clamp(controls.sun_intensity, 0.0f, 2.5f);
    controls.environment_strength = std::clamp(controls.environment_strength, 0.0f, 1.5f);
    controls.fog_density = std::clamp(controls.fog_density, 0.0f, 0.05f);
    controls.fog_height_falloff = std::clamp(controls.fog_height_falloff, 0.0f, 0.5f);
    controls.animation_speed = std::clamp(controls.animation_speed, 0.0f, 4.0f);
    controls.particle_strength = std::clamp(controls.particle_strength, 0.0f, 4.0f);
    controls.ssao_strength = std::clamp(controls.ssao_strength, 0.0f, 2.0f);
    controls.ssao_radius = std::clamp(controls.ssao_radius, 0.25f, 3.0f);
    controls.clearcoat_strength = std::clamp(controls.clearcoat_strength, 0.0f, 2.0f);
    controls.transmission_strength = std::clamp(controls.transmission_strength, 0.0f, 2.0f);
    controls.projector_strength = std::clamp(controls.projector_strength, 0.0f, 2.5f);
    controls.tessellation_level = std::clamp(controls.tessellation_level, 1.0f, 32.0f);
    controls.water_wave_strength = std::clamp(controls.water_wave_strength, 0.0f, 3.0f);
    controls.water_refraction_strength = std::clamp(controls.water_refraction_strength, 0.0f, 2.0f);
    controls.cloth_wind_strength = std::clamp(controls.cloth_wind_strength, 0.0f, 2.5f);
    controls.foliage_density = std::clamp(controls.foliage_density, 0.25f, 2.0f);
    controls.day_night_speed = std::clamp(controls.day_night_speed, 0.05f, 6.0f);
    controls.bench_explode = std::clamp(controls.bench_explode, 0.0f, 2.0f);
    controls.gui_scale = std::clamp(controls.gui_scale, 1.0f, 5.0f);
}

void disable_tier1(context::RuntimeControls& controls) noexcept {
    controls.ssao = false;
    controls.tessellation = false;
    controls.pn_triangles = false;
    controls.indirect_draw = false;
    controls.gpu_queries = false;
}
}

GuiOverlay::GuiOverlay(const std::filesystem::path& root, render::ResourceSpine& resources)
    : resources_{resources},
      shader_{root / "shaders/gui/gui.vert", root / "shaders/gui/gui.frag"},
      font_{root / "gui/font_ascii.png", gl::ColorSpace::linear, 1.0f, false, false, gl::clamp_to_edge},
      editor_save_path_{root / "editor/default_scene.cfg"},
      grass_mask_path_{root / "editor/grass_placement_mask.pgm"} {
    gl::GenVertexArrays(1, &vao_);
    gl::GenBuffers(1, &vbo_);
    gl::BindVertexArray(vao_);
    gl::BindBuffer(gl::array_buffer, vbo_);
    constexpr GLsizei stride = sizeof(Vertex);
    gl::EnableVertexAttribArray(0);
    gl::VertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(offsetof(Vertex, position)));
    gl::EnableVertexAttribArray(1);
    gl::VertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(offsetof(Vertex, uv)));
    gl::EnableVertexAttribArray(2);
    gl::VertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(offsetof(Vertex, color)));
    gl::EnableVertexAttribArray(3);
    gl::VertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(offsetof(Vertex, textured)));
    gl::BindVertexArray(0);

    gl::BindTexture(GL_TEXTURE_2D, font_.id());
    gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    gl::BindTexture(GL_TEXTURE_2D, 0);
}

GuiOverlay::~GuiOverlay() {
    if (vbo_) gl::DeleteBuffers(1, &vbo_);
    if (vao_) gl::DeleteVertexArrays(1, &vao_);
}

void GuiOverlay::initialize_scene_editor(render::scene::SceneSpine& scene) {
    // The typed hardcoded table is the authoritative baseline. Snapshot it first,
    // then layer the editable cfg over it so Reset never depends on external data.
    authored_object_count_ = scene.objects.size();
    authored_material_count_ = scene.materials.size();
    editor_defaults_.clear();
    editor_defaults_.reserve(scene.objects.size());
    for (const auto& object : scene.objects) {
        editor_defaults_.push_back({object.transform, object.authored_position,
                                    object.authored_size, object.relative_scale,
                                    object.mesh, object.material,
                                    editor_properties(scene, object),
                                    object.visible, object.editor_deleted});
    }

    load_scene_overrides(scene);
    load_foliage_masks(scene);
    foliage_mask_defaults_.clear();
    foliage_mask_defaults_.reserve(scene.foliage_regions.size());
    for (const auto& region : scene.foliage_regions)
        foliage_mask_defaults_.push_back(region.placement_mask);
    sync_all_bound_entities(scene);
}

GuiOverlay::EditorImportRequest GuiOverlay::consume_import_request() noexcept {
    const EditorImportRequest requested = import_request_;
    import_request_ = EditorImportRequest::none;
    return requested;
}

void GuiOverlay::apply_imported_mesh(render::scene::SceneSpine& scene, render::MeshHandle mesh,
                                     std::string_view display_name) {
    if (selected_object_ == invalid_editor_object || selected_object_ >= scene.objects.size()) return;
    auto existing = std::ranges::find_if(scene.editor_models, [&](const auto& option) {
        return option.mesh == mesh;
    });
    if (existing == scene.editor_models.end()) {
        scene.editor_models.push_back({std::string(display_name), mesh});
        selected_model_option_ = scene.editor_models.size() - 1u;
    } else {
        selected_model_option_ = static_cast<std::size_t>(std::distance(scene.editor_models.begin(), existing));
    }
    for (const auto index : selected_indices(scene)) {
        if (index < scene.objects.size() && !scene.objects[index].editor_only)
            scene.objects[index].mesh = mesh;
    }
}

render::MaterialHandle GuiOverlay::ensure_unique_material(render::scene::SceneSpine& scene,
                                                           std::size_t object_index) {
    if (object_index >= scene.objects.size()) return {};
    auto& object = scene.objects[object_index];
    if (!object.material || object.material.value > scene.materials.size()) return {};
    const auto current_handle = object.material;
    const auto& current = scene.material(current_handle);
    const bool shared = std::ranges::count_if(scene.objects, [&](const auto& candidate) {
        return candidate.material == current_handle;
    }) > 1;
    if (current.editor_unique && !shared) return current_handle;

    auto clone = current;
    clone.name = "Editor: " + object.name;
    clone.editor_unique = true;
    if (!current.editor_unique) {
        clone.editor_base_material_name = current.name;
        clone.editor_albedo_asset.clear();
        clone.editor_normal_asset.clear();
        clone.editor_orm_asset.clear();
    }
    object.material = scene.add_material(std::move(clone));
    selected_material_option_ = static_cast<std::size_t>(object.material.value - 1u);
    return object.material;
}

void GuiOverlay::reset_selected_texture_slot(render::scene::SceneSpine& scene) {
    for (const auto index : selected_indices(scene)) {
        if (index >= scene.objects.size() || scene.objects[index].editor_only) continue;
        auto& object = scene.objects[index];
        if (!object.material || object.material.value > scene.materials.size()) continue;
        auto& edited = scene.material(object.material);
        if (!edited.editor_unique || edited.editor_base_material_name.empty()) continue;
        const auto base_handle = material_handle_by_name(scene, edited.editor_base_material_name);
        if (!base_handle) continue;
        const auto& base = scene.material(base_handle);
        switch (selected_texture_slot_) {
        case EditorImportRequest::normal_texture:
            edited.normal = base.normal;
            edited.editor_normal_asset.clear();
            break;
        case EditorImportRequest::orm_texture:
            edited.orm = base.orm;
            edited.editor_orm_asset.clear();
            break;
        case EditorImportRequest::albedo_texture:
        default:
            edited.albedo = base.albedo;
            edited.editor_albedo_asset.clear();
            break;
        }
    }
}

void GuiOverlay::apply_imported_texture(render::scene::SceneSpine& scene,
                                        render::TextureHandle texture,
                                        EditorImportRequest slot,
                                        std::string_view display_name,
                                        std::string_view relative_asset_path) {
    const auto indices = selected_indices(scene);
    if (indices.empty()) return;
    render::MaterialHandle primary_material{};
    for (const auto index : indices) {
        if (index >= scene.objects.size() || scene.objects[index].editor_only) continue;
        const auto unique_material = ensure_unique_material(scene, index);
        if (!unique_material) continue;
        if (!primary_material) primary_material = unique_material;

        auto& material = scene.material(unique_material);
        material.name = std::string(display_name) + " [" + scene.objects[index].name + "]";
        switch (slot) {
        case EditorImportRequest::normal_texture:
            material.normal = texture;
            material.editor_normal_asset = std::string(relative_asset_path);
            break;
        case EditorImportRequest::orm_texture:
            material.orm = texture;
            material.editor_orm_asset = std::string(relative_asset_path);
            break;
        case EditorImportRequest::albedo_texture:
        default:
            material.albedo = texture;
            material.editor_albedo_asset = std::string(relative_asset_path);
            break;
        }
    }
    if (primary_material) selected_material_option_ = primary_material.value - 1u;
}

void GuiOverlay::synchronize_scene_editor(render::scene::SceneSpine& scene) {
    sync_all_bound_entities(scene);
}

std::vector<std::size_t> GuiOverlay::selected_indices(const render::scene::SceneSpine& scene) const {
    std::vector<std::size_t> result;
    if (manual_multiselect_ && !multi_selected_objects_.empty()) {
        for (const auto index : multi_selected_objects_) {
            if (index < scene.objects.size() && !scene.objects[index].editor_deleted)
                result.push_back(index);
        }
        return result;
    }
    if (selected_object_ == invalid_editor_object || selected_object_ >= scene.objects.size()) return result;
    const auto& selected = scene.objects[selected_object_];
    if (selected.editor_deleted) return result;
    if (!editor_group_mode_ || selected.editor_group.empty()) {
        result.push_back(selected_object_);
        return result;
    }
    for (std::size_t index = 0; index < scene.objects.size(); ++index) {
        const auto& object = scene.objects[index];
        if (!object.editor_deleted && object.editor_group == selected.editor_group)
            result.push_back(index);
    }
    return result;
}

void GuiOverlay::deselect_all() {
    active_numeric_edit_ = invalid_editor_object;
    active_numeric_scrub_ = invalid_editor_object;
    numeric_scrub_dragged_ = false;
    numeric_edit_buffer_.clear();
    selected_object_ = invalid_editor_object;
    multi_selected_objects_.clear();
    manual_multiselect_ = false;
    editor_group_mode_ = false;
    inspector_.open = false;
}

void GuiOverlay::refresh_editor_selection(render::scene::SceneSpine& scene) {
    const auto indices = selected_indices(scene);
    if (indices.empty()) {
        deselect_all();
        return;
    }
    if (indices.size() > 1u) {
        math::Vec3 pivot{};
        for (const auto object_index : indices) pivot += scene.objects[object_index].authored_position;
        pivot = pivot / static_cast<float>(indices.size());
        editor_position_ = pivot;
        editor_rotation_degrees_ = {};
        editor_size_ = {1.0f, 1.0f, 1.0f};
        editor_scale_ = {1.0f, 1.0f, 1.0f};
    } else {
        selected_object_ = indices.front();
        const auto& object = scene.objects[selected_object_];
        editor_position_ = object.authored_position;
        editor_rotation_degrees_ = degrees(object.transform.rotation);
        editor_size_ = object.authored_size;
        editor_scale_ = object.relative_scale;
    }
    previous_editor_position_ = editor_position_;
    previous_editor_rotation_degrees_ = editor_rotation_degrees_;
    previous_editor_size_ = editor_size_;
    previous_editor_scale_ = editor_scale_;

    selected_model_option_ = 0;
    for (std::size_t option = 0; option < scene.editor_models.size(); ++option) {
        if (scene.editor_models[option].mesh == scene.objects[selected_object_].mesh) {
            selected_model_option_ = option;
            break;
        }
    }
    selected_material_option_ = scene.objects[selected_object_].material
        ? static_cast<std::size_t>(scene.objects[selected_object_].material.value - 1u) : 0u;
    if (!scene.materials.empty()) selected_material_option_ = std::min(selected_material_option_, scene.materials.size() - 1u);
}

void GuiOverlay::select_object(render::scene::SceneSpine& scene, std::size_t index) {
    if (index >= scene.objects.size() || scene.objects[index].editor_deleted) return;
    active_numeric_edit_ = invalid_editor_object;
    active_numeric_scrub_ = invalid_editor_object;
    numeric_scrub_dragged_ = false;
    numeric_edit_buffer_.clear();
    selected_object_ = index;
    multi_selected_objects_.clear();
    manual_multiselect_ = false;
    editor_group_mode_ = false;
    inspector_.open = true;
    inspector_.close_pressed = false;
    refresh_editor_selection(scene);
}

void GuiOverlay::select_object_at(const context::InputState& input,
                                  const context::FrameContext& frame,
                                  const context::RuntimeControls& controls,
                                  render::scene::SceneSpine& scene) {
    if (frame.framebuffer_width <= 0 || frame.framebuffer_height <= 0) return;
    const float aspect = static_cast<float>(frame.framebuffer_width)
        / static_cast<float>(frame.framebuffer_height);
    const math::Mat4 view_projection = scene.camera.projection(aspect) * scene.camera.view();
    const math::Vec2 mouse{static_cast<float>(input.mouse_x), static_cast<float>(input.mouse_y)};

    const bool effect_only = input.keys[18];
    float best_score = std::numeric_limits<float>::max();
    std::size_t best_index = invalid_editor_object;
    for (std::size_t index = 0; index < scene.objects.size(); ++index) {
        const auto& object = scene.objects[index];
        if (object.editor_deleted || object.camera_attached) continue;
        if (object.editor_deleted) continue;
        const bool hidden_debug = controls.scene_debug_view && controls.debug_hidden_objects && !object.visible;
        const bool effect_debug = controls.scene_debug_view && controls.debug_effect_bounds && object.editor_only;
        if (!object.visible && !hidden_debug && !effect_debug) continue;
        if (object.editor_only && !effect_debug && !effect_only) continue;
        if (effect_only && !object.editor_only) continue;
        const math::Vec3 pick_position = object.transform.position + object.editor_pick_offset;
        const math::Vec4 clip = view_projection * math::Vec4{
            pick_position.x, pick_position.y, pick_position.z, 1.0f
        };
        if (clip.w <= 0.05f) continue;
        const float inverse_w = 1.0f / clip.w;
        const float ndc_x = clip.x * inverse_w;
        const float ndc_y = clip.y * inverse_w;
        const float ndc_z = clip.z * inverse_w;
        if (ndc_z < -1.0f || ndc_z > 1.0f || std::abs(ndc_x) > 1.2f || std::abs(ndc_y) > 1.2f) continue;

        const math::Vec2 screen{
            (ndc_x * 0.5f + 0.5f) * static_cast<float>(frame.framebuffer_width),
            (1.0f - (ndc_y * 0.5f + 0.5f)) * static_cast<float>(frame.framebuffer_height)
        };
        const float dx = screen.x - mouse.x;
        const float dy = screen.y - mouse.y;
        const float pixel_distance = std::sqrt(dx * dx + dy * dy);
        const float distance = math::length(pick_position - scene.camera.position);
        const float extent = std::max({std::abs(object.transform.scale.x),
                                       std::abs(object.transform.scale.y),
                                       std::abs(object.transform.scale.z)})
            * std::max(object.editor_pick_radius_scale, 0.1f);
        const float minimum_radius = object.editor_only ? 24.0f : 10.0f;
        const float radius = std::clamp(extent * 260.0f / std::max(distance, 0.5f), minimum_radius, 220.0f);
        if (pixel_distance > radius) continue;
        const float score = pixel_distance + distance * 0.035f - (object.editor_only ? 4.0f : 0.0f);
        if (score < best_score) {
            best_score = score;
            best_index = index;
        }
    }

    // Terrain is renderer-owned rather than an ordinary PBR object. When no
    // normal object/effect wins the click, intersect the cursor ray with its base
    // plane and select the terrain proxy anywhere inside its editable footprint.
    if (best_index == invalid_editor_object && !effect_only && scene.terrain.visible) {
        const float ndc_x = mouse.x / static_cast<float>(frame.framebuffer_width) * 2.0f - 1.0f;
        const float ndc_y = 1.0f - mouse.y / static_cast<float>(frame.framebuffer_height) * 2.0f;
        const math::Vec3 forward = scene.camera.forward();
        const math::Vec3 right = math::normalize(math::cross(forward, math::Vec3{0.0f, 1.0f, 0.0f}));
        const math::Vec3 up = math::normalize(math::cross(right, forward));
        const float tangent = std::tan(math::radians(scene.camera.fov_degrees) * 0.5f);
        const math::Vec3 ray = math::normalize(forward
            + right * (ndc_x * aspect * tangent) + up * (ndc_y * tangent));
        if (std::abs(ray.y) > 0.0001f) {
            const float distance = (scene.terrain.transform.position.y - scene.camera.position.y) / ray.y;
            if (distance > 0.0f) {
                const math::Vec3 hit = scene.camera.position + ray * distance;
                const float dx = hit.x - scene.terrain.transform.position.x;
                const float dz = hit.z - scene.terrain.transform.position.z;
                const float cosine = std::cos(scene.terrain.transform.rotation.y);
                const float sine = std::sin(scene.terrain.transform.rotation.y);
                const float local_x = (cosine * dx - sine * dz)
                    / std::max(std::abs(scene.terrain.transform.scale.x), 0.001f);
                const float local_z = (sine * dx + cosine * dz)
                    / std::max(std::abs(scene.terrain.transform.scale.z), 0.001f);
                if (std::abs(local_x) <= 1.0f && std::abs(local_z) <= 1.0f) {
                    const auto terrain_proxy = std::ranges::find_if(scene.objects, [](const auto& candidate) {
                        return candidate.editor_binding == render::scene::EditorBindingKind::terrain_surface
                            && !candidate.editor_deleted;
                    });
                    if (terrain_proxy != scene.objects.end())
                        best_index = static_cast<std::size_t>(std::distance(scene.objects.begin(), terrain_proxy));
                }
            }
        }
    }

    const bool additive = input.keys[16] || input.keys[17];
    if (best_index == invalid_editor_object) {
        if (!additive) deselect_all();
        return;
    }
    if (!additive) {
        select_object(scene, best_index);
        return;
    }

    if (!manual_multiselect_) {
        multi_selected_objects_ = selected_indices(scene);
        manual_multiselect_ = true;
        editor_group_mode_ = false;
    }
    const auto existing = std::find(multi_selected_objects_.begin(), multi_selected_objects_.end(), best_index);
    if (existing == multi_selected_objects_.end()) {
        multi_selected_objects_.push_back(best_index);
        selected_object_ = best_index;
    } else {
        multi_selected_objects_.erase(existing);
        if (selected_object_ == best_index)
            selected_object_ = multi_selected_objects_.empty() ? invalid_editor_object : multi_selected_objects_.back();
    }
    if (multi_selected_objects_.empty()) {
        deselect_all();
        return;
    }
    inspector_.open = true;
    inspector_.close_pressed = false;
    refresh_editor_selection(scene);
}

void GuiOverlay::sync_bound_entity(render::scene::SceneSpine& scene, std::size_t object_index) {
    if (object_index >= scene.objects.size()) return;
    auto& object = scene.objects[object_index];
    using render::scene::EditorBindingKind;
    switch (object.editor_binding) {
    case EditorBindingKind::water_surface:
        if (object.editor_binding_index < scene.water_surfaces.size()) {
            auto& value = scene.water_surfaces[object.editor_binding_index];
            object.authored_size.y = 1.0f;
            object.relative_scale.y = 1.0f;
            object.transform.scale.y = 1.0f;
            value.transform = object.transform;
            value.visible = object.visible;
        }
        break;
    case EditorBindingKind::cloth_object:
        if (object.editor_binding_index < scene.cloth_objects.size()) {
            auto& value = scene.cloth_objects[object.editor_binding_index];
            value.origin = object.transform.position;
            value.width = std::max(object.transform.scale.x, 0.25f);
            value.height = std::max(object.transform.scale.y, 0.25f);
            value.visible = object.visible;
        }
        break;
    case EditorBindingKind::rtt_display:
        if (object.editor_binding_index < scene.rtt_displays.size()) {
            auto& value = scene.rtt_displays[object.editor_binding_index];
            value.transform = object.transform;
            value.visible = object.visible;
        }
        break;
    case EditorBindingKind::scene_label:
        if (object.editor_binding_index < scene.labels.size()) {
            auto& value = scene.labels[object.editor_binding_index];
            value.world_position = object.transform.position;
            value.visible = object.visible;
        }
        break;
    case EditorBindingKind::point_light:
        if (object.editor_binding_index < scene.point_light_count) {
            scene.point_lights[object.editor_binding_index].position = object.transform.position;
            set_editor_properties(scene, object, editor_properties(scene, object));
            for (auto& peer : scene.objects) {
                if (&peer == &object || peer.editor_binding != object.editor_binding
                    || peer.editor_binding_index != object.editor_binding_index) continue;
                peer.authored_position = object.authored_position;
                peer.transform.position = object.transform.position;
                peer.editor_properties = object.editor_properties;
                peer.visible = object.visible;
                peer.editor_deleted = object.editor_deleted;
            }
        }
        break;
    case EditorBindingKind::spotlight: {
        auto& light = spotlight_at(scene, object.editor_binding_index);
        light.position = object.transform.position;
        const bool physical_lamp = object.name.find("spotlight lamp") != std::string::npos;
        if (!physical_lamp) {
            light.authored_direction = math::normalize(
                rotate_editor_offset({0.0f, 0.0f, -1.0f}, object.transform.rotation));
            light.direction = light.authored_direction;
        }
        set_editor_properties(scene, object, editor_properties(scene, object));
        for (auto& peer : scene.objects) {
            if (&peer == &object || peer.editor_binding != object.editor_binding
                || peer.editor_binding_index != object.editor_binding_index) continue;
            peer.authored_position = object.authored_position;
            peer.transform.position = object.transform.position;
            if (!physical_lamp) peer.transform.rotation = object.transform.rotation;
            peer.editor_properties = object.editor_properties;
            peer.visible = object.visible;
            peer.editor_deleted = object.editor_deleted;
        }
        break;
    }
    case EditorBindingKind::terrain_surface:
        object.transform.rotation.x = 0.0f;
        object.transform.rotation.z = 0.0f;
        scene.terrain.transform = object.transform;
        scene.terrain.visible = object.visible;
        set_editor_properties(scene, object, editor_properties(scene, object));
        break;
    case EditorBindingKind::foliage_region:
        if (object.editor_binding_index < scene.foliage_regions.size()) {
            auto& value = scene.foliage_regions[object.editor_binding_index];
            object.authored_size.y = 1.0f;
            object.relative_scale.y = 1.0f;
            object.transform.scale.y = 1.0f;
            value.transform = object.transform;
            value.visible = object.visible;
        }
        break;
    case EditorBindingKind::particle_emitter:
        if (object.editor_binding_index < scene.particle_emitters.size()) {
            auto& value = scene.particle_emitters[object.editor_binding_index];
            value.transform.position = object.transform.position;
            value.transform.rotation = object.transform.rotation;
            value.size_scale = std::clamp(object.editor_properties[1], 0.0f, 6.0f);
            value.intensity = std::clamp(object.editor_properties[0], 0.0f, 4.0f);
            value.visible = object.visible;
        }
        break;
    default:
        break;
    }
}

void GuiOverlay::sync_all_bound_entities(render::scene::SceneSpine& scene) {
    for (std::size_t index = 0; index < scene.objects.size(); ++index) sync_bound_entity(scene, index);
}

void GuiOverlay::apply_editor_transform(render::scene::SceneSpine& scene) {
    const auto indices = selected_indices(scene);
    if (indices.empty()) return;

    const auto& primary = scene.objects[selected_object_];
    editor_position_.x = std::clamp(editor_position_.x, primary.editor_bounds_min.x, primary.editor_bounds_max.x);
    editor_position_.y = std::clamp(editor_position_.y, primary.editor_bounds_min.y, primary.editor_bounds_max.y);
    editor_position_.z = std::clamp(editor_position_.z, primary.editor_bounds_min.z, primary.editor_bounds_max.z);
    editor_size_.x = std::clamp(editor_size_.x, primary.editor_min_scale.x, primary.editor_max_scale.x);
    editor_size_.y = std::clamp(editor_size_.y, primary.editor_min_scale.y, primary.editor_max_scale.y);
    editor_size_.z = std::clamp(editor_size_.z, primary.editor_min_scale.z, primary.editor_max_scale.z);
    editor_scale_.x = std::clamp(editor_scale_.x, 0.01f, 50.0f);
    editor_scale_.y = std::clamp(editor_scale_.y, 0.01f, 50.0f);
    editor_scale_.z = std::clamp(editor_scale_.z, 0.01f, 50.0f);
    if (primary.editor_lock_scale_y) {
        editor_size_.y = previous_editor_size_.y;
        editor_scale_.y = previous_editor_scale_.y;
    }
    if (primary.editor_binding == render::scene::EditorBindingKind::water_surface
        || primary.editor_binding == render::scene::EditorBindingKind::foliage_region) {
        editor_rotation_degrees_.x = 0.0f;
        editor_rotation_degrees_.z = 0.0f;
    }

    if (indices.size() == 1u) {
        auto& object = scene.objects[indices.front()];
        object.authored_position = editor_position_;
        if (!object.camera_attached) object.transform.position = editor_position_;
        object.transform.rotation = radians(editor_rotation_degrees_);
        object.authored_size = editor_size_;
        object.relative_scale = editor_scale_;
        object.transform.scale = object.authored_size * object.relative_scale;
        object.transform.scale.x = std::clamp(object.transform.scale.x, object.editor_min_scale.x, object.editor_max_scale.x);
        object.transform.scale.y = std::clamp(object.transform.scale.y, object.editor_min_scale.y, object.editor_max_scale.y);
        object.transform.scale.z = std::clamp(object.transform.scale.z, object.editor_min_scale.z, object.editor_max_scale.z);
        object.relative_scale = {
            safe_ratio(object.transform.scale.x, object.authored_size.x),
            safe_ratio(object.transform.scale.y, object.authored_size.y),
            safe_ratio(object.transform.scale.z, object.authored_size.z)};
        editor_scale_ = object.relative_scale;
    } else {
        const math::Vec3 translation = editor_position_ - previous_editor_position_;
        for (const auto index : indices) {
            auto& object = scene.objects[index];
            object.authored_position += translation;
            if (!object.camera_attached) object.transform.position = object.authored_position;
        }

        const math::Vec3 scale_ratio{
            safe_ratio(editor_scale_.x, previous_editor_scale_.x),
            safe_ratio(editor_scale_.y, previous_editor_scale_.y),
            safe_ratio(editor_scale_.z, previous_editor_scale_.z)
        };
        for (const auto index : indices) {
            auto& object = scene.objects[index];
            const math::Vec3 offset = object.authored_position - editor_position_;
            object.authored_position = editor_position_ + offset * scale_ratio;
            object.transform.position = object.authored_position;
            object.relative_scale = object.relative_scale * scale_ratio;
            if (object.editor_lock_scale_y) object.relative_scale.y = 1.0f;
            object.transform.scale = object.authored_size * object.relative_scale;
            object.transform.scale.x = std::clamp(object.transform.scale.x, object.editor_min_scale.x, object.editor_max_scale.x);
            object.transform.scale.y = std::clamp(object.transform.scale.y, object.editor_min_scale.y, object.editor_max_scale.y);
            object.transform.scale.z = std::clamp(object.transform.scale.z, object.editor_min_scale.z, object.editor_max_scale.z);
            object.relative_scale = {
                safe_ratio(object.transform.scale.x, object.authored_size.x),
                safe_ratio(object.transform.scale.y, object.authored_size.y),
                safe_ratio(object.transform.scale.z, object.authored_size.z)};
            object.explode_direction = object.explode_direction * scale_ratio;
        }

        const math::Vec3 rotation_delta = radians(editor_rotation_degrees_ - previous_editor_rotation_degrees_);
        for (const auto index : indices) {
            auto& object = scene.objects[index];
            const math::Vec3 offset = object.authored_position - editor_position_;
            object.authored_position = editor_position_ + rotate_editor_offset(offset, rotation_delta);
            object.transform.position = object.authored_position;
            object.transform.rotation += rotation_delta;
            object.explode_direction = rotate_editor_offset(object.explode_direction, rotation_delta);
        }
    }

    for (const auto index : indices) sync_bound_entity(scene, index);
    previous_editor_position_ = editor_position_;
    previous_editor_rotation_degrees_ = editor_rotation_degrees_;
    previous_editor_size_ = editor_size_;
    previous_editor_scale_ = editor_scale_;
}

void GuiOverlay::duplicate_selected(render::scene::SceneSpine& scene) {
    const auto sources = selected_indices(scene);
    if (sources.empty()) return;

    ++duplicate_serial_;
    const std::string suffix = " Copy " + std::to_string(duplicate_serial_);
    std::vector<std::size_t> duplicates;
    duplicates.reserve(sources.size());

    for (const auto source_index : sources) {
        if (source_index >= scene.objects.size()) continue;
        const auto& source = scene.objects[source_index];
        if (source.editor_only || source.editor_binding != render::scene::EditorBindingKind::none) continue;

        auto copy = source;
        copy.name += suffix;
        if (!copy.editor_group.empty()) copy.editor_group += suffix;
        copy.editor_duplicate_source = source_index;
        copy.authored_position.x += 0.75f;
        copy.transform.position = copy.authored_position;
        copy.editor_deleted = false;
        copy.visible = true;

        scene.objects.push_back(std::move(copy));
        const std::size_t new_index = scene.objects.size() - 1u;
        const auto& added = scene.objects[new_index];
        editor_defaults_.push_back({added.transform, added.authored_position,
                                    added.authored_size, added.relative_scale, added.mesh, added.material,
                                    editor_properties(scene, added), added.visible, added.editor_deleted});
        duplicates.push_back(new_index);
    }

    if (duplicates.empty()) return;
    multi_selected_objects_ = duplicates;
    manual_multiselect_ = duplicates.size() > 1u;
    editor_group_mode_ = false;
    selected_object_ = duplicates.back();
    inspector_.open = true;
    refresh_editor_selection(scene);
}

void GuiOverlay::save_scene_overrides(const render::scene::SceneSpine& scene) const {
    std::error_code error;
    std::filesystem::create_directories(editor_save_path_.parent_path(), error);
    const std::filesystem::path temporary_path = editor_save_path_.string() + ".tmp";
    std::ofstream output(temporary_path, std::ios::trunc);
    if (!output) return;
    output << "EPOCH_SCENE_EDITOR_V5\n";
    output << std::setprecision(9);
    for (std::size_t index = 0; index < scene.objects.size(); ++index) {
        const auto& object = scene.objects[index];
        std::string model_name;
        for (const auto& option : scene.editor_models) {
            if (option.mesh == object.mesh) {
                model_name = option.name;
                break;
            }
        }

        std::string base_material_name;
        std::string albedo_asset;
        std::string normal_asset;
        std::string orm_asset;
        float uv_scale = 1.0f;
        float metallic = 1.0f;
        float roughness = 1.0f;
        float normal_scale = 1.0f;
        if (object.material && object.material.value <= scene.materials.size()) {
            const auto& material = scene.material(object.material);
            base_material_name = material.editor_unique && !material.editor_base_material_name.empty()
                ? material.editor_base_material_name : material.name;
            albedo_asset = material.editor_albedo_asset;
            normal_asset = material.editor_normal_asset;
            orm_asset = material.editor_orm_asset;
            uv_scale = material.uv_scale;
            metallic = material.metallic_factor;
            roughness = material.roughness_factor;
            normal_scale = material.normal_scale;
        }

        const auto properties = editor_properties(scene, object);
        const long long duplicate_source = object.editor_duplicate_source == invalid_editor_object
            ? -1ll : static_cast<long long>(object.editor_duplicate_source);
        output << index << ' ' << std::quoted(object.name) << ' '
               << object.authored_position.x << ' ' << object.authored_position.y << ' ' << object.authored_position.z << ' '
               << object.transform.rotation.x << ' ' << object.transform.rotation.y << ' ' << object.transform.rotation.z << ' '
               << object.authored_size.x << ' ' << object.authored_size.y << ' ' << object.authored_size.z << ' '
               << object.relative_scale.x << ' ' << object.relative_scale.y << ' ' << object.relative_scale.z << ' '
               << std::quoted(model_name) << ' ' << (object.visible ? 1 : 0) << ' '
               << (object.editor_deleted ? 1 : 0) << ' '
               << properties[0] << ' ' << properties[1] << ' ' << properties[2] << ' ' << properties[3] << ' '
               << std::quoted(base_material_name) << ' '
               << std::quoted(albedo_asset) << ' ' << std::quoted(normal_asset) << ' ' << std::quoted(orm_asset) << ' '
               << uv_scale << ' ' << metallic << ' ' << roughness << ' ' << normal_scale << ' '
               << duplicate_source << ' ' << std::quoted(object.editor_group) << '\n';
    }
    output.flush();
    if (!output) {
        output.close();
        std::filesystem::remove(temporary_path, error);
        return;
    }
    output.close();
    if (!replace_file_atomically(temporary_path, editor_save_path_)) {
        std::filesystem::remove(temporary_path, error);
        return;
    }
    save_foliage_masks(scene);
}

void GuiOverlay::save_foliage_masks(const render::scene::SceneSpine& scene) const {
    std::error_code error;
    std::filesystem::create_directories(grass_mask_path_.parent_path(), error);
    for (std::size_t index = 0; index < scene.foliage_regions.size(); ++index) {
        const std::filesystem::path path = index == 0u ? grass_mask_path_
            : grass_mask_path_.parent_path() /
                (grass_mask_path_.stem().string() + "_" + std::to_string(index + 1u) + ".pgm");
        const std::filesystem::path temporary = path.string() + ".tmp";
        std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
        if (!output) continue;
        output << "P5\n" << render::scene::foliage_mask_resolution << ' '
               << render::scene::foliage_mask_resolution << "\n255\n";
        const auto& mask = scene.foliage_regions[index].placement_mask;
        output.write(reinterpret_cast<const char*>(mask.data()),
                     static_cast<std::streamsize>(mask.size()));
        output.close();
        if (!replace_file_atomically(temporary, path))
            std::filesystem::remove(temporary, error);
    }
}

void GuiOverlay::load_foliage_masks(render::scene::SceneSpine& scene) {
    for (std::size_t index = 0; index < scene.foliage_regions.size(); ++index) {
        const std::filesystem::path path = index == 0u ? grass_mask_path_
            : grass_mask_path_.parent_path() /
                (grass_mask_path_.stem().string() + "_" + std::to_string(index + 1u) + ".pgm");
        std::ifstream input(path, std::ios::binary);
        if (!input) continue;
        std::string magic;
        int width{};
        int height{};
        int maximum{};
        if (!(input >> magic >> width >> height >> maximum) || magic != "P5"
            || width != static_cast<int>(render::scene::foliage_mask_resolution)
            || height != static_cast<int>(render::scene::foliage_mask_resolution)
            || maximum != 255) continue;
        input.get();
        auto& region = scene.foliage_regions[index];
        input.read(reinterpret_cast<char*>(region.placement_mask.data()),
                   static_cast<std::streamsize>(region.placement_mask.size()));
        if (input.gcount() == static_cast<std::streamsize>(region.placement_mask.size()))
            ++region.placement_mask_revision;
        else
            region.placement_mask = render::scene::full_foliage_mask();
    }
}

void GuiOverlay::load_scene_overrides(render::scene::SceneSpine& scene) {
    if (scene.objects.size() > authored_object_count_) scene.objects.resize(authored_object_count_);
    if (editor_defaults_.size() > authored_object_count_) editor_defaults_.resize(authored_object_count_);
    if (scene.materials.size() > authored_material_count_) scene.materials.resize(authored_material_count_);
    multi_selected_objects_.clear();
    manual_multiselect_ = false;
    if (selected_object_ >= scene.objects.size()) selected_object_ = invalid_editor_object;

    for (std::size_t index = 0; index < scene.objects.size() && index < editor_defaults_.size(); ++index) {
        auto& object = scene.objects[index];
        const auto& snapshot = editor_defaults_[index];
        object.transform = snapshot.transform;
        object.authored_position = snapshot.authored_position;
        object.authored_size = snapshot.authored_size;
        object.relative_scale = snapshot.relative_scale;
        object.mesh = snapshot.mesh;
        object.material = snapshot.material;
        object.visible = snapshot.visible;
        object.editor_deleted = snapshot.deleted;
        object.editor_duplicate_source = invalid_editor_object;
        set_editor_properties(scene, object, snapshot.properties);
    }

    std::ifstream input(editor_save_path_);
    if (!input) {
        sync_all_bound_entities(scene);
        return;
    }
    std::string header;
    std::getline(input, header);
    const bool version_five = header == "EPOCH_SCENE_EDITOR_V5";
    const bool version_four = header == "EPOCH_SCENE_EDITOR_V4";
    const bool version_three = header == "EPOCH_SCENE_EDITOR_V3";
    const bool version_two = header == "EPOCH_SCENE_EDITOR_V2";
    if (!version_five && !version_four && !version_three && !version_two
        && header != "EPOCH_SCENE_EDITOR_V1") {
        sync_all_bound_entities(scene);
        return;
    }

    while (input) {
        std::size_t index{};
        std::string name;
        math::Vec3 authored{};
        math::Vec3 rotation{};
        math::Vec3 authored_size{1.0f, 1.0f, 1.0f};
        math::Vec3 relative_scale{1.0f, 1.0f, 1.0f};
        std::string model_name;
        int enabled{};
        int deleted{};
        if (!(input >> index >> std::quoted(name)
              >> authored.x >> authored.y >> authored.z
              >> rotation.x >> rotation.y >> rotation.z
              >> authored_size.x >> authored_size.y >> authored_size.z)) break;
        if (version_five || version_four) {
            if (!(input >> relative_scale.x >> relative_scale.y >> relative_scale.z)) break;
        }
        if (!(input >> std::quoted(model_name) >> enabled)) break;
        if (version_five) {
            if (!(input >> deleted)) break;
        } else {
            deleted = enabled == 0 ? 1 : 0;
        }

        std::array<float, 4> properties{};
        if (version_two || version_three || version_four || version_five) {
            if (!(input >> properties[0] >> properties[1] >> properties[2] >> properties[3])) break;
        }

        std::string base_material_name;
        std::string albedo_asset;
        std::string normal_asset;
        std::string orm_asset;
        float uv_scale = 1.0f;
        float metallic = 1.0f;
        float roughness = 1.0f;
        float normal_scale = 1.0f;
        std::string editor_group;
        long long duplicate_source = -1;
        if (version_five || version_four) {
            if (!(input >> std::quoted(base_material_name)
                  >> std::quoted(albedo_asset) >> std::quoted(normal_asset) >> std::quoted(orm_asset)
                  >> uv_scale >> metallic >> roughness >> normal_scale
                  >> duplicate_source >> std::quoted(editor_group))) break;
        } else if (version_three) {
            if (!(input >> std::quoted(base_material_name) >> duplicate_source >> std::quoted(editor_group))) break;
        }

        if (index >= scene.objects.size()) {
            if ((!version_three && !version_four && !version_five) || index != scene.objects.size() || duplicate_source < 0
                || static_cast<std::size_t>(duplicate_source) >= scene.objects.size()) continue;
            const auto source_index = static_cast<std::size_t>(duplicate_source);
            const auto& source = scene.objects[source_index];
            if (source.editor_only || source.editor_binding != render::scene::EditorBindingKind::none) continue;
            auto copy = source;
            copy.name = name;
            copy.editor_group = editor_group;
            copy.editor_duplicate_source = source_index;
            scene.objects.push_back(std::move(copy));
        }
        if (index >= scene.objects.size()) continue;
        auto& object = scene.objects[index];
        if (index < authored_object_count_ && object.name != name) continue;
        if (index >= authored_object_count_) {
            object.name = name;
            object.editor_group = editor_group;
            object.editor_duplicate_source = duplicate_source < 0
                ? invalid_editor_object : static_cast<std::size_t>(duplicate_source);
        }

        object.authored_position = authored;
        object.transform.position = authored;
        object.transform.rotation = rotation;
        object.authored_size = authored_size;
        object.relative_scale = relative_scale;
        object.transform.scale = object.authored_size * object.relative_scale;
        object.editor_deleted = deleted != 0;
        object.visible = enabled != 0 && !object.editor_deleted;

        if (!model_name.empty()) {
            const auto option = std::ranges::find_if(scene.editor_models, [&](const auto& candidate) {
                return candidate.name == model_name;
            });
            if (option != scene.editor_models.end()) object.mesh = option->mesh;
        }

        if (!base_material_name.empty()) {
            const auto base_handle = material_handle_by_name(scene, base_material_name);
            if (base_handle) {
                object.material = base_handle;
                if (version_five || version_four) {
                    const auto& base = scene.material(base_handle);
                    const bool customized = !albedo_asset.empty() || !normal_asset.empty() || !orm_asset.empty()
                        || different(uv_scale, base.uv_scale)
                        || different(metallic, base.metallic_factor)
                        || different(roughness, base.roughness_factor)
                        || different(normal_scale, base.normal_scale);
                    if (customized) {
                        auto edited = base;
                        edited.name = "Editor: " + object.name;
                        edited.editor_unique = true;
                        edited.editor_base_material_name = base.name;
                        edited.editor_albedo_asset = albedo_asset;
                        edited.editor_normal_asset = normal_asset;
                        edited.editor_orm_asset = orm_asset;
                        edited.uv_scale = uv_scale;
                        edited.metallic_factor = metallic;
                        edited.roughness_factor = roughness;
                        edited.normal_scale = normal_scale;
                        const auto reload_texture = [&](std::string_view asset, gl::ColorSpace color_space,
                                                        render::TextureHandle& destination) {
                            if (asset.empty()) return;
                            try {
                                destination = resources_.load_texture(asset, color_space);
                            } catch (const std::exception&) {
                                // Preserve the base slot. The saved path remains so a later reload can retry.
                            }
                        };
                        reload_texture(albedo_asset, gl::ColorSpace::srgb, edited.albedo);
                        reload_texture(normal_asset, gl::ColorSpace::linear, edited.normal);
                        reload_texture(orm_asset, gl::ColorSpace::linear, edited.orm);
                        object.material = scene.add_material(std::move(edited));
                    }
                }
            }
        }
        if (version_two || version_three || version_four || version_five)
            set_editor_properties(scene, object, properties);
        if (index >= editor_defaults_.size()) {
            editor_defaults_.push_back({object.transform, object.authored_position,
                                        object.authored_size, object.relative_scale,
                                        object.mesh, object.material,
                                        editor_properties(scene, object),
                                        object.visible, object.editor_deleted});
        }
    }
    sync_all_bound_entities(scene);
    if (selected_object_ != invalid_editor_object && selected_object_ < scene.objects.size())
        refresh_editor_selection(scene);
}

void GuiOverlay::reset_selected(render::scene::SceneSpine& scene) {
    const auto indices = selected_indices(scene);
    for (const auto index : indices) {
        if (index >= editor_defaults_.size() || index >= scene.objects.size()) continue;
        auto& object = scene.objects[index];
        const auto& snapshot = editor_defaults_[index];
        object.transform = snapshot.transform;
        object.authored_position = snapshot.authored_position;
        object.authored_size = snapshot.authored_size;
        object.relative_scale = snapshot.relative_scale;
        object.mesh = snapshot.mesh;
        object.material = snapshot.material;
        object.visible = snapshot.visible;
        object.editor_deleted = snapshot.deleted;
        set_editor_properties(scene, object, snapshot.properties);
        sync_bound_entity(scene, index);
        if (object.editor_binding == render::scene::EditorBindingKind::foliage_region
            && object.editor_binding_index < scene.foliage_regions.size()
            && object.editor_binding_index < foliage_mask_defaults_.size()) {
            auto& region = scene.foliage_regions[object.editor_binding_index];
            region.placement_mask = foliage_mask_defaults_[object.editor_binding_index];
            ++region.placement_mask_revision;
        }
    }
    if (selected_object_ != invalid_editor_object && selected_object_ < scene.objects.size())
        refresh_editor_selection(scene);
}

void GuiOverlay::update_inspector(context::InputState& input,
                                  const context::FrameContext& frame,
                                  render::scene::SceneSpine& scene,
                                  egl::Vec2 mouse,
                                  const egl::FloatingWindowInput& window_input) {
    if (selected_object_ == invalid_editor_object || selected_object_ >= scene.objects.size()) return;
    const float inverse_scale = 1.0f / ui_scale_;
    const egl::Vec2 logical_viewport{
        static_cast<float>(frame.framebuffer_width) * inverse_scale,
        static_cast<float>(frame.framebuffer_height) * inverse_scale
    };
    const float panel_right = panel_.open ? panel_layout_.window.position.x + panel_layout_.window.size.x + 12.0f : 14.0f;
    const float default_x = std::max(panel_right, logical_viewport.x - 470.0f);
    inspector_.open = true;
    const egl::FloatingWindowOptions options{
        {default_x, 34.0f}, {460.0f, 720.0f}, {430.0f, 620.0f}, logical_viewport,
        30.0f, 10.0f, true, true, false
    };
    inspector_layout_ = egl::update_floating_window(inspector_, options, window_input);

    const egl::ButtonInput button_input{mouse, input.left_mouse_down,
                                        input.left_mouse_pressed, input.left_mouse_released};
    float x = inspector_layout_.content.position.x;
    float y = inspector_layout_.content.position.y + 54.0f;
    const float width = inspector_layout_.content.size.x;

    const egl::Rect previous_rect{{x, y}, {86.0f, 28.0f}};
    const egl::Rect next_rect{{x + 92.0f, y}, {86.0f, 28.0f}};
    const auto previous = egl::update_button(inspector_button_states_[0], previous_rect, button_input);
    const auto next = egl::update_button(inspector_button_states_[1], next_rect, button_input);
    const bool has_group = !scene.objects[selected_object_].editor_group.empty();
    const egl::Rect selection_mode_rect{{x + 184.0f, y}, {std::max(0.0f, width - 184.0f), 28.0f}};
    const auto selection_mode = has_group && !manual_multiselect_
        ? egl::update_button(inspector_button_states_[7], selection_mode_rect, button_input)
        : egl::ButtonLayout{};
    const auto adjacent_object = [&](int direction) {
        if (scene.objects.empty()) return invalid_editor_object;
        std::size_t candidate = selected_object_;
        for (std::size_t attempt = 0; attempt < scene.objects.size(); ++attempt) {
            candidate = direction < 0
                ? (candidate == 0u ? scene.objects.size() - 1u : candidate - 1u)
                : (candidate + 1u) % scene.objects.size();
            if (!scene.objects[candidate].editor_deleted) return candidate;
        }
        return invalid_editor_object;
    };
    if (previous.activated) {
        const auto target = adjacent_object(-1);
        if (target != invalid_editor_object) select_object(scene, target);
        return;
    }
    if (next.activated) {
        const auto target = adjacent_object(1);
        if (target != invalid_editor_object) select_object(scene, target);
        return;
    }
    if (has_group && !manual_multiselect_ && selection_mode.activated) {
        editor_group_mode_ = !editor_group_mode_;
        select_object(scene, selected_object_);
        return;
    }
    y += 34.0f;

    const egl::Rect visibility_rect{{x, y}, {width, 30.0f}};
    const auto enabled_indices = selected_indices(scene);
    bool enabled_value = std::ranges::all_of(enabled_indices, [&](std::size_t index) {
        return index < scene.objects.size() && scene.objects[index].visible
            && !scene.objects[index].editor_deleted;
    });
    const auto enabled_toggle = egl::update_toggle(enabled_value, visibility_rect, button_input);
    if (enabled_toggle.changed) {
        for (const auto index : enabled_indices) {
            if (index >= scene.objects.size()) continue;
            auto& object = scene.objects[index];
            if (object.editor_deleted) continue;
            object.visible = enabled_value;
            sync_bound_entity(scene, index);
        }
    }
    y += 42.0f;

    const float page_half = (width - 6.0f) * 0.5f;
    const auto transform_page = egl::update_button(inspector_button_states_[13], {{x, y}, {page_half, 28.0f}}, button_input);
    const auto asset_page = egl::update_button(inspector_button_states_[14], {{x + page_half + 6.0f, y}, {page_half, 28.0f}}, button_input);
    if (transform_page.activated) {
        inspector_page_ = 0u;
        active_numeric_edit_ = invalid_editor_object;
        active_numeric_scrub_ = invalid_editor_object;
        numeric_scrub_dragged_ = false;
        numeric_edit_buffer_.clear();
    }
    if (asset_page.activated) {
        inspector_page_ = 1u;
        active_numeric_edit_ = invalid_editor_object;
        active_numeric_scrub_ = invalid_editor_object;
        numeric_scrub_dragged_ = false;
        numeric_edit_buffer_.clear();
    }
    y += 34.0f;

    const bool shift = input.keys[16];
    const bool control = input.keys[17];
    const auto parse_numeric_edit = [&](float& value, float low, float high) {
        if (numeric_edit_buffer_.empty() || numeric_edit_buffer_ == "-" || numeric_edit_buffer_ == "+"
            || numeric_edit_buffer_ == "." || numeric_edit_buffer_ == "-." || numeric_edit_buffer_ == "+.") return;
        char* end = nullptr;
        const float parsed = std::strtof(numeric_edit_buffer_.c_str(), &end);
        if (end != numeric_edit_buffer_.c_str() && end && *end == '\0' && std::isfinite(parsed))
            value = std::clamp(parsed, low, high);
    };
    const auto edit_scalar = [&](float& value, float low, float high, float finite_step,
                                 std::size_t control_index, std::size_t edit_id) {
        const float multiplier = shift ? 10.0f : (control ? 0.1f : 1.0f);
        const float step = finite_step * multiplier;
        const egl::Rect minus_rect{{x + 78.0f, y}, {28.0f, 24.0f}};
        const egl::Rect value_rect{{x + 112.0f, y}, {112.0f, 24.0f}};
        const egl::Rect plus_rect{{x + 230.0f, y}, {28.0f, 24.0f}};
        const bool value_hovered = egl::contains(value_rect, mouse);

        if (value_hovered || active_numeric_scrub_ == edit_id)
            input.cursor_shape = context::CursorShape::horizontal_resize;

        const auto minus = egl::update_button(inspector_scalar_button_states_[control_index * 2u],
                                              minus_rect, button_input);
        const auto plus = egl::update_button(inspector_scalar_button_states_[control_index * 2u + 1u],
                                             plus_rect, button_input);

        if (input.left_mouse_pressed && value_hovered) {
            active_numeric_scrub_ = edit_id;
            numeric_scrub_start_value_ = value;
            numeric_scrub_start_mouse_x_ = mouse.x;
            numeric_scrub_dragged_ = false;
            active_numeric_edit_ = invalid_editor_object;
            numeric_edit_buffer_.clear();
        }

        if (active_numeric_scrub_ == edit_id && input.left_mouse_down) {
            const float drag_pixels = mouse.x - numeric_scrub_start_mouse_x_;
            if (std::abs(drag_pixels) >= 2.0f) {
                numeric_scrub_dragged_ = true;
                // Editor-style number scrubbing: Shift is precision, Ctrl is coarse,
                // and Shift+Ctrl reaches hundredth-step precision.
                const float scrub_multiplier = shift && control ? 0.01f
                    : (shift ? 0.1f : (control ? 10.0f : 1.0f));
                const float magnitude = std::max(std::abs(numeric_scrub_start_value_), finite_step);
                const float decimal_place = std::pow(10.0f, std::floor(std::log10(magnitude)));
                const float adaptive_step = std::clamp(decimal_place * 0.01f, 0.01f, 1000.0f);
                const float units_per_pixel = std::max(finite_step, adaptive_step)
                    * scrub_multiplier * 0.125f;
                value = std::clamp(numeric_scrub_start_value_ + drag_pixels * units_per_pixel, low, high);
            }
        }

        if (active_numeric_scrub_ == edit_id && input.left_mouse_released) {
            active_numeric_scrub_ = invalid_editor_object;
            if (!numeric_scrub_dragged_) {
                active_numeric_edit_ = edit_id;
                char buffer[48]{};
                std::snprintf(buffer, sizeof(buffer), "%.7g", value);
                numeric_edit_buffer_ = buffer;
                input.cursor_shape = context::CursorShape::text;
            }
            numeric_scrub_dragged_ = false;
        }

        if (active_numeric_edit_ == edit_id) {
            if (value_hovered) input.cursor_shape = context::CursorShape::text;
            bool commit = false;
            for (std::size_t index = 0; index < input.text_input_count; ++index) {
                const char character = input.text_input[index];
                if (character == '\r') {
                    commit = true;
                    continue;
                }
                if (character == '\b') {
                    if (!numeric_edit_buffer_.empty()) numeric_edit_buffer_.pop_back();
                    continue;
                }
                const bool allowed = (character >= '0' && character <= '9')
                    || character == '.' || character == '-' || character == '+';
                if (allowed && numeric_edit_buffer_.size() < 31u)
                    numeric_edit_buffer_.push_back(character);
            }
            if (input.text_input_count > 0u) {
                parse_numeric_edit(value, low, high);
                input.text_input_count = 0u;
            }
            if (input.left_mouse_pressed && !value_hovered
                && !egl::contains(minus_rect, mouse) && !egl::contains(plus_rect, mouse)) {
                commit = true;
            }
            if (commit) {
                parse_numeric_edit(value, low, high);
                active_numeric_edit_ = invalid_editor_object;
                numeric_edit_buffer_.clear();
            }
        }

        if (minus.activated || plus.activated) {
            if (active_numeric_edit_ == edit_id) parse_numeric_edit(value, low, high);
            value = std::clamp(value + (plus.activated ? step : -step), low, high);
            active_numeric_edit_ = invalid_editor_object;
            active_numeric_scrub_ = invalid_editor_object;
            numeric_edit_buffer_.clear();
        } else if (input.mouse_wheel != 0 && value_hovered) {
            value = std::clamp(value + step * static_cast<float>(input.mouse_wheel), low, high);
            if (active_numeric_edit_ == edit_id) {
                char buffer[48]{};
                std::snprintf(buffer, sizeof(buffer), "%.7g", value);
                numeric_edit_buffer_ = buffer;
            }
        }

        value = std::clamp(value, low, high);
        y += 29.0f;
    };

    const auto active_indices = selected_indices(scene);
    if (inspector_page_ == 0u) {
    const auto& editor_limits = scene.objects[selected_object_];
    edit_scalar(editor_position_.x, editor_limits.editor_bounds_min.x, editor_limits.editor_bounds_max.x, 0.10f, 0u, 0u);
    edit_scalar(editor_position_.y, editor_limits.editor_bounds_min.y, editor_limits.editor_bounds_max.y, 0.10f, 1u, 1u);
    edit_scalar(editor_position_.z, editor_limits.editor_bounds_min.z, editor_limits.editor_bounds_max.z, 0.10f, 2u, 2u);

    using render::scene::EditorBindingKind;
    const auto binding = editor_limits.editor_binding;
    const bool transform_rotation = binding == EditorBindingKind::none
        || binding == EditorBindingKind::water_surface
        || binding == EditorBindingKind::cloth_object
        || binding == EditorBindingKind::rtt_display
        || binding == EditorBindingKind::foliage_region;
    const bool transform_size = transform_rotation || binding == EditorBindingKind::terrain_surface;

    if (transform_rotation) {
        edit_scalar(editor_rotation_degrees_.x, -180.0f, 180.0f, 1.0f, 3u, 3u);
        edit_scalar(editor_rotation_degrees_.y, -180.0f, 180.0f, 1.0f, 4u, 4u);
        edit_scalar(editor_rotation_degrees_.z, -180.0f, 180.0f, 1.0f, 5u, 5u);
    }
    if (transform_size && selected_indices(scene).size() == 1u) {
        edit_scalar(editor_size_.x, editor_limits.editor_min_scale.x, editor_limits.editor_max_scale.x, 0.10f, 6u, 6u);
        edit_scalar(editor_size_.y, editor_limits.editor_min_scale.y, editor_limits.editor_max_scale.y, 0.10f, 7u, 7u);
        edit_scalar(editor_size_.z, editor_limits.editor_min_scale.z, editor_limits.editor_max_scale.z, 0.10f, 8u, 8u);
    }
    if (transform_size) {
        edit_scalar(editor_scale_.x, 0.01f, 50.0f, 0.05f, 9u, 9u);
        edit_scalar(editor_scale_.y, 0.01f, 50.0f, 0.05f, 10u, 10u);
        edit_scalar(editor_scale_.z, 0.01f, 50.0f, 0.05f, 11u, 11u);
    }
    apply_editor_transform(scene);

    if (active_indices.size() == 1u) {
        auto& selected = scene.objects[active_indices.front()];
        auto properties = editor_properties(scene, selected);
        switch (selected.editor_binding) {
        case EditorBindingKind::water_surface:
            edit_scalar(properties[0], 0.0f, 0.5f, 0.01f, 12u, 12u);
            edit_scalar(properties[1], 0.0f, 4.0f, 0.05f, 13u, 13u);
            edit_scalar(properties[2], 0.0f, 1.0f, 0.01f, 14u, 14u);
            edit_scalar(properties[3], 0.0f, 1.0f, 0.01f, 15u, 15u);
            break;
        case EditorBindingKind::cloth_object:
            edit_scalar(properties[0], 0.0f, 4.0f, 0.05f, 12u, 12u);
            edit_scalar(properties[1], 0.0f, 2.0f, 0.02f, 13u, 13u);
            edit_scalar(properties[2], 0.0f, 1.0f, 0.01f, 14u, 14u);
            break;
        case EditorBindingKind::point_light:
            edit_scalar(properties[0], 0.25f, 50.0f, 0.25f, 12u, 12u);
            edit_scalar(properties[1], 0.0f, 80.0f, 0.50f, 13u, 13u);
            break;
        case EditorBindingKind::spotlight:
            edit_scalar(properties[0], 0.25f, 80.0f, 0.25f, 12u, 12u);
            edit_scalar(properties[1], 0.0f, 100.0f, 0.50f, 13u, 13u);
            edit_scalar(properties[2], 0.0f, 1.0f, 0.01f, 14u, 14u);
            edit_scalar(properties[3], 0.0f, 1.0f, 0.01f, 15u, 15u);
            break;
        case EditorBindingKind::terrain_surface:
            edit_scalar(properties[0], 0.0f, 24.0f, 0.10f, 12u, 12u);
            break;
        case EditorBindingKind::foliage_region:
            edit_scalar(properties[0], 0.0f, 4.0f, 0.05f, 12u, 12u);
            edit_scalar(properties[1], 0.0f, 4.0f, 0.05f, 13u, 13u);
            edit_scalar(properties[2], 0.0f, 6.0f, 0.05f, 14u, 14u);
            edit_scalar(properties[3], 0.0f, 4.0f, 0.05f, 15u, 15u);
            break;
        case EditorBindingKind::particle_emitter:
            edit_scalar(properties[0], 0.0f, 4.0f, 0.05f, 12u, 12u);
            edit_scalar(properties[1], 0.0f, 6.0f, 0.05f, 13u, 13u);
            break;
        default:
            break;
        }
        set_editor_properties(scene, selected, properties);
        sync_bound_entity(scene, active_indices.front());
    }
    }

    if (inspector_page_ == 1u && !scene.objects[selected_object_].editor_only) {
        const egl::Rect model_row{{x, y}, {width, 30.0f}};
        if (input.left_mouse_released && egl::contains(model_row, mouse) && !scene.editor_models.empty())
            selected_model_option_ = (selected_model_option_ + 1u) % scene.editor_models.size();
        y += 36.0f;

        const float half = (width - 6.0f) * 0.5f;
        const auto replace = egl::update_button(inspector_button_states_[2], {{x, y}, {half, 30.0f}}, button_input);
        const auto import_model = egl::update_button(inspector_button_states_[3], {{x + half + 6.0f, y}, {half, 30.0f}}, button_input);
        if (replace.activated && selected_model_option_ < scene.editor_models.size()) {
            for (const auto index : selected_indices(scene)) {
                if (index < scene.objects.size() && !scene.objects[index].editor_only)
                    scene.objects[index].mesh = scene.editor_models[selected_model_option_].mesh;
            }
        }
        if (import_model.activated) import_request_ = EditorImportRequest::obj_model;
        y += 38.0f;

        const egl::Rect material_row{{x, y}, {width, 30.0f}};
        if (input.left_mouse_released && egl::contains(material_row, mouse) && !scene.materials.empty())
            selected_material_option_ = (selected_material_option_ + 1u) % scene.materials.size();
        y += 36.0f;

        const egl::Rect texture_slot_row{{x, y}, {width, 30.0f}};
        if (input.left_mouse_released && egl::contains(texture_slot_row, mouse)) {
            if (selected_texture_slot_ == EditorImportRequest::albedo_texture)
                selected_texture_slot_ = EditorImportRequest::normal_texture;
            else if (selected_texture_slot_ == EditorImportRequest::normal_texture)
                selected_texture_slot_ = EditorImportRequest::orm_texture;
            else
                selected_texture_slot_ = EditorImportRequest::albedo_texture;
        }
        y += 36.0f;

        const float material_third = (width - 12.0f) / 3.0f;
        const auto apply_material = egl::update_button(inspector_button_states_[10], {{x, y}, {material_third, 30.0f}}, button_input);
        const auto load_texture = egl::update_button(inspector_button_states_[11], {{x + material_third + 6.0f, y}, {material_third, 30.0f}}, button_input);
        const auto reset_texture = egl::update_button(inspector_button_states_[15], {{x + (material_third + 6.0f) * 2.0f, y}, {material_third, 30.0f}}, button_input);
        if (apply_material.activated && selected_material_option_ < scene.materials.size()) {
            const render::MaterialHandle material{static_cast<std::uint32_t>(selected_material_option_ + 1u)};
            for (const auto index : selected_indices(scene)) {
                if (index < scene.objects.size() && !scene.objects[index].editor_only)
                    scene.objects[index].material = material;
            }
        }
        if (load_texture.activated) import_request_ = selected_texture_slot_;
        if (reset_texture.activated) reset_selected_texture_slot(scene);
        y += 38.0f;

        if (active_indices.size() == 1u) {
            const auto object_index = active_indices.front();
            auto& object = scene.objects[object_index];
            if (object.material && object.material.value <= scene.materials.size()) {
                const auto& current = scene.material(object.material);
                float uv = current.uv_scale;
                float metallic = current.metallic_factor;
                float roughness = current.roughness_factor;
                float normal = current.normal_scale;
                edit_scalar(uv, 0.05f, 20.0f, 0.05f, 16u, 16u);
                edit_scalar(metallic, 0.0f, 1.0f, 0.01f, 17u, 17u);
                edit_scalar(roughness, 0.0f, 1.0f, 0.01f, 18u, 18u);
                edit_scalar(normal, 0.0f, 4.0f, 0.05f, 19u, 19u);
                if (different(uv, current.uv_scale) || different(metallic, current.metallic_factor)
                    || different(roughness, current.roughness_factor) || different(normal, current.normal_scale)) {
                    const auto handle = ensure_unique_material(scene, object_index);
                    if (handle) {
                        auto& edited = scene.material(handle);
                        edited.uv_scale = uv;
                        edited.metallic_factor = metallic;
                        edited.roughness_factor = roughness;
                        edited.normal_scale = normal;
                    }
                }
            }
        }
    }

    if (inspector_page_ == 1u
        && scene.objects[selected_object_].editor_binding == render::scene::EditorBindingKind::foliage_region
        && scene.objects[selected_object_].editor_binding_index < scene.foliage_regions.size()) {
        auto& region = scene.foliage_regions[scene.objects[selected_object_].editor_binding_index];
        const float third_mask = (width - 12.0f) / 3.0f;
        const auto paint_mode = egl::update_button(inspector_button_states_[16],
            {{x, y}, {third_mask, 30.0f}}, button_input);
        const auto erase_mode = egl::update_button(inspector_button_states_[17],
            {{x + third_mask + 6.0f, y}, {third_mask, 30.0f}}, button_input);
        const auto allow_all = egl::update_button(inspector_button_states_[18],
            {{x + (third_mask + 6.0f) * 2.0f, y}, {third_mask, 30.0f}}, button_input);
        if (paint_mode.activated) grass_paint_add_ = true;
        if (erase_mode.activated) grass_paint_add_ = false;
        if (allow_all.activated) {
            region.placement_mask = render::scene::full_foliage_mask();
            ++region.placement_mask_revision;
        }
        y += 38.0f;
        edit_scalar(grass_brush_radius_, 1.0f, 16.0f, 1.0f, 20u, 2020u);

        const float canvas_size = std::min(width, 300.0f);
        const egl::Rect canvas{{x, y}, {canvas_size, canvas_size}};
        if ((input.left_mouse_down || input.right_mouse_down) && egl::contains(canvas, mouse)) {
            const float u = std::clamp((mouse.x - canvas.position.x) / canvas.size.x, 0.0f, 0.999999f);
            const float v = std::clamp(1.0f - (mouse.y - canvas.position.y) / canvas.size.y, 0.0f, 0.999999f);
            const int resolution = static_cast<int>(render::scene::foliage_mask_resolution);
            const int center_x = std::clamp(static_cast<int>(u * resolution), 0, resolution - 1);
            const int center_y = std::clamp(static_cast<int>(v * resolution), 0, resolution - 1);
            const int radius = std::max(1, static_cast<int>(std::lround(grass_brush_radius_)));
            const std::uint8_t value = input.right_mouse_down || !grass_paint_add_ ? 0u : 255u;
            bool changed = false;
            for (int offset_y = -radius; offset_y <= radius; ++offset_y) {
                for (int offset_x = -radius; offset_x <= radius; ++offset_x) {
                    if (offset_x * offset_x + offset_y * offset_y > radius * radius) continue;
                    const int cell_x = center_x + offset_x;
                    const int cell_y = center_y + offset_y;
                    if (cell_x < 0 || cell_x >= resolution || cell_y < 0 || cell_y >= resolution) continue;
                    auto& cell = region.placement_mask[static_cast<std::size_t>(cell_y)
                        * render::scene::foliage_mask_resolution + static_cast<std::size_t>(cell_x)];
                    if (cell == value) continue;
                    cell = value;
                    changed = true;
                }
            }
            if (changed) ++region.placement_mask_revision;
            captures_mouse_ = true;
        }
        y += canvas_size + 30.0f;
    }

    const bool can_duplicate = std::ranges::any_of(selected_indices(scene), [&](std::size_t index) {
        return index < scene.objects.size() && !scene.objects[index].editor_only
            && scene.objects[index].editor_binding == render::scene::EditorBindingKind::none;
    });
    const float third = (width - 12.0f) / 3.0f;
    const auto duplicate = egl::update_button(inspector_button_states_[12], {{x, y}, {third, 30.0f}}, button_input);
    const auto remove = egl::update_button(inspector_button_states_[8], {{x + third + 6.0f, y}, {third, 30.0f}}, button_input);
    const auto deselect = egl::update_button(inspector_button_states_[9], {{x + (third + 6.0f) * 2.0f, y}, {third, 30.0f}}, button_input);
    y += 36.0f;

    const float third2 = (width - 12.0f) / 3.0f;
    const auto save = egl::update_button(inspector_button_states_[4], {{x, y}, {third2, 30.0f}}, button_input);
    const auto reload = egl::update_button(inspector_button_states_[5], {{x + third2 + 6.0f, y}, {third2, 30.0f}}, button_input);
    const auto reset = egl::update_button(inspector_button_states_[6], {{x + (third2 + 6.0f) * 2.0f, y}, {third2, 30.0f}}, button_input);

    if (duplicate.activated && can_duplicate) duplicate_selected(scene);
    if (save.activated) save_scene_overrides(scene);
    if (reload.activated) {
        load_scene_overrides(scene);
        load_foliage_masks(scene);
    }
    if (reset.activated) reset_selected(scene);
    if (remove.activated) {
        const auto targets = selected_indices(scene);
        for (const auto index : targets) {
            if (index >= scene.objects.size()) continue;
            auto& object = scene.objects[index];
            object.visible = false;
            object.editor_deleted = true;
            sync_bound_entity(scene, index);
        }
        deselect_all();
    }
    if (deselect.activated) deselect_all();
}

bool GuiOverlay::update(context::InputState& input, const context::FrameContext& frame,
                        context::RuntimeControls& controls,
                        render::scene::SceneSpine& scene) {
    input.cursor_shape = context::CursorShape::arrow;
    clamp_controls(controls);
    ui_scale_ = controls.gui_scale;

    const float inverse_scale = 1.0f / ui_scale_;
    const egl::Vec2 mouse{
        static_cast<float>(input.mouse_x) * inverse_scale,
        static_cast<float>(input.mouse_y) * inverse_scale
    };

    if (!controls.show_gui) {
        captures_mouse_ = egl::contains(reopen_button_, mouse);
        if (input.left_mouse_released && captures_mouse_) {
            controls.show_gui = true;
            panel_.open = true;
            panel_.close_pressed = false;
        }
        const egl::FloatingWindowInput inspector_input{
            mouse, input.left_mouse_down, input.left_mouse_pressed, input.left_mouse_released
        };
        if (selected_object_ != invalid_editor_object && selected_object_ < scene.objects.size()) {
            update_inspector(input, frame, scene, mouse, inspector_input);
            captures_mouse_ = captures_mouse_ || inspector_layout_.hovered
                || inspector_.dragging || inspector_.resizing
                || active_numeric_scrub_ != invalid_editor_object;
        }
        if (input.left_mouse_released && !captures_mouse_)
            select_object_at(input, frame, controls, scene);
        return captures_mouse_;
    }

    if (!panel_.open) {
        panel_.open = true;
        panel_.close_pressed = false;
    }

    const egl::Vec2 logical_viewport{
        static_cast<float>(frame.framebuffer_width) * inverse_scale,
        static_cast<float>(frame.framebuffer_height) * inverse_scale
    };
    panel_.size.x = std::min(panel_.size.x, std::max(260.0f, logical_viewport.x - 10.0f));
    panel_.size.y = std::min(panel_.size.y, std::max(160.0f, logical_viewport.y - 10.0f));

    const egl::FloatingWindowOptions options{
        {14.0f, 34.0f}, {520.0f, 600.0f}, {340.0f, 220.0f}, logical_viewport,
        30.0f, 10.0f, true, true, true
    };
    const egl::FloatingWindowInput window_input{
        mouse, input.left_mouse_down, input.left_mouse_pressed, input.left_mouse_released
    };
    panel_layout_ = egl::update_floating_window(panel_, options, window_input);
    if (!panel_.open) {
        controls.show_gui = false;
        captures_mouse_ = false;
        if (selected_object_ != invalid_editor_object && selected_object_ < scene.objects.size()) {
            update_inspector(input, frame, scene, mouse, window_input);
            captures_mouse_ = inspector_layout_.hovered || inspector_.dragging || inspector_.resizing
                || active_numeric_scrub_ != invalid_editor_object;
        }
        return captures_mouse_;
    }

    captures_mouse_ = panel_layout_.hovered || panel_.dragging || panel_.resizing
        || active_numeric_scrub_ != invalid_editor_object;
    const egl::ButtonInput button_input{
        window_input.mouse_position, window_input.mouse_down,
        window_input.mouse_pressed, window_input.mouse_released
    };

    float x = panel_layout_.content.position.x;
    float y = panel_layout_.content.position.y + 34.0f;
    const float width = panel_layout_.content.size.x;
    const float tab_width = std::max(44.0f, (width - 8.0f) / static_cast<float>(tab_count));
    const std::array<float, tab_count> tab_widths{tab_width, tab_width, tab_width, tab_width, tab_width};
    egl::SegmentedControlLayoutOptions tabs{{x, y}, tab_widths.data(), static_cast<std::uint32_t>(tab_count), 25.0f, 2.0f};
    if (input.left_mouse_released) {
        const auto tab = egl::segmented_control_item_at(tabs, window_input.mouse_position);
        if (tab != egl::invalid_selectable_row_index) {
            selected_tab_ = std::min<std::uint32_t>(tab, tab_count - 1u);
            active_numeric_edit_ = invalid_editor_object;
            active_numeric_scrub_ = invalid_editor_object;
            numeric_scrub_dragged_ = false;
            numeric_edit_buffer_.clear();
        }
    }
    y += 38.0f;

    const float content_bottom = panel_layout_.window.position.y + panel_layout_.window.size.y - 34.0f;
    const float visible_height = std::max(30.0f, content_bottom - y);
    const float maximum_scroll = std::max(0.0f, tab_content_height(selected_tab_) - visible_height);
    if (panel_layout_.hovered && input.mouse_wheel != 0) {
        tab_scroll_[selected_tab_] = std::clamp(
            tab_scroll_[selected_tab_] - static_cast<float>(input.mouse_wheel) * 48.0f,
            0.0f, maximum_scroll);
    } else {
        tab_scroll_[selected_tab_] = std::clamp(tab_scroll_[selected_tab_], 0.0f, maximum_scroll);
    }
    y -= tab_scroll_[selected_tab_];

    const auto section = [&]() { y += section_height; };
    const auto toggle = [&](bool& value) {
        const egl::Rect rect{{x, y}, {width, row_height}};
        (void)egl::update_toggle(value, rect, button_input);
        y += row_height;
    };
    const auto advanced_toggle = [&](bool& value) {
        const bool previous = value;
        toggle(value);
        if (!previous && value) controls.tier0_mobile_profile = false;
    };
    const bool shift = input.keys[16];
    const bool control = input.keys[17];
    const auto parse_numeric_edit = [&](float& value, float low, float high) {
        if (numeric_edit_buffer_.empty() || numeric_edit_buffer_ == "-" || numeric_edit_buffer_ == "+"
            || numeric_edit_buffer_ == "." || numeric_edit_buffer_ == "-." || numeric_edit_buffer_ == "+.") return;
        char* end = nullptr;
        const float parsed = std::strtof(numeric_edit_buffer_.c_str(), &end);
        if (end != numeric_edit_buffer_.c_str() && end && *end == '\0' && std::isfinite(parsed))
            value = std::clamp(parsed, low, high);
    };
    const auto scalar = [&](float& value, float low, float high, float finite_step, std::size_t index) {
        const float multiplier = shift ? 10.0f : (control ? 0.1f : 1.0f);
        const float step = finite_step * multiplier;
        const float controls_x = x + width - 190.0f;
        const egl::Rect minus_rect{{controls_x, y + 2.0f}, {28.0f, 24.0f}};
        const egl::Rect value_rect{{controls_x + 34.0f, y + 2.0f}, {122.0f, 24.0f}};
        const egl::Rect plus_rect{{controls_x + 162.0f, y + 2.0f}, {28.0f, 24.0f}};
        const std::size_t edit_id = tuning_edit_base + index;
        const bool value_hovered = egl::contains(value_rect, mouse);

        if (value_hovered || active_numeric_scrub_ == edit_id)
            input.cursor_shape = context::CursorShape::horizontal_resize;

        const auto minus = egl::update_button(tuning_scalar_button_states_[index * 2u], minus_rect, button_input);
        const auto plus = egl::update_button(tuning_scalar_button_states_[index * 2u + 1u], plus_rect, button_input);

        if (input.left_mouse_pressed && value_hovered) {
            active_numeric_scrub_ = edit_id;
            numeric_scrub_start_value_ = value;
            numeric_scrub_start_mouse_x_ = mouse.x;
            numeric_scrub_dragged_ = false;
            active_numeric_edit_ = invalid_editor_object;
            numeric_edit_buffer_.clear();
        }

        if (active_numeric_scrub_ == edit_id && input.left_mouse_down) {
            const float drag_pixels = mouse.x - numeric_scrub_start_mouse_x_;
            if (std::abs(drag_pixels) >= 2.0f) {
                numeric_scrub_dragged_ = true;
                const float scrub_multiplier = shift && control ? 0.01f
                    : (shift ? 0.1f : (control ? 10.0f : 1.0f));
                const float magnitude = std::max(std::abs(numeric_scrub_start_value_), finite_step);
                const float decimal_place = std::pow(10.0f, std::floor(std::log10(magnitude)));
                const float adaptive_step = std::clamp(decimal_place * 0.01f, 0.01f, 1000.0f);
                const float units_per_pixel = std::max(finite_step, adaptive_step)
                    * scrub_multiplier * 0.125f;
                value = std::clamp(numeric_scrub_start_value_ + drag_pixels * units_per_pixel, low, high);
            }
        }

        if (active_numeric_scrub_ == edit_id && input.left_mouse_released) {
            active_numeric_scrub_ = invalid_editor_object;
            if (!numeric_scrub_dragged_) {
                active_numeric_edit_ = edit_id;
                char buffer[48]{};
                std::snprintf(buffer, sizeof(buffer), "%.7g", value);
                numeric_edit_buffer_ = buffer;
                input.cursor_shape = context::CursorShape::text;
            }
            numeric_scrub_dragged_ = false;
        }

        if (active_numeric_edit_ == edit_id) {
            if (value_hovered) input.cursor_shape = context::CursorShape::text;
            bool commit = false;
            for (std::size_t character_index = 0; character_index < input.text_input_count; ++character_index) {
                const char character = input.text_input[character_index];
                if (character == '\r') {
                    commit = true;
                    continue;
                }
                if (character == '\b') {
                    if (!numeric_edit_buffer_.empty()) numeric_edit_buffer_.pop_back();
                    continue;
                }
                const bool allowed = (character >= '0' && character <= '9')
                    || character == '.' || character == '-' || character == '+';
                if (allowed && numeric_edit_buffer_.size() < 31u)
                    numeric_edit_buffer_.push_back(character);
            }
            if (input.text_input_count > 0u) {
                parse_numeric_edit(value, low, high);
                input.text_input_count = 0u;
            }
            if (input.left_mouse_pressed && !value_hovered
                && !egl::contains(minus_rect, mouse) && !egl::contains(plus_rect, mouse)) {
                commit = true;
            }
            if (commit) {
                parse_numeric_edit(value, low, high);
                active_numeric_edit_ = invalid_editor_object;
                numeric_edit_buffer_.clear();
            }
        }

        if (minus.activated || plus.activated) {
            if (active_numeric_edit_ == edit_id) parse_numeric_edit(value, low, high);
            value = std::clamp(value + (plus.activated ? step : -step), low, high);
            active_numeric_edit_ = invalid_editor_object;
            active_numeric_scrub_ = invalid_editor_object;
            numeric_edit_buffer_.clear();
        }
        value = std::clamp(value, low, high);
        y += numeric_row_height;
    };

    if (selected_tab_ == 0) {
        const egl::Rect scene_row{{x, y}, {width, row_height}};
        if (input.left_mouse_released && egl::contains(scene_row, window_input.mouse_position))
            controls.scene_preset = static_cast<context::ScenePreset>((static_cast<unsigned>(controls.scene_preset) + 1u) % 5u);
        y += row_height;
        const egl::Rect mode_row{{x, y}, {width, row_height}};
        if (input.left_mouse_released && egl::contains(mode_row, window_input.mouse_position))
            controls.shading_mode = static_cast<context::ShadingMode>((static_cast<unsigned>(controls.shading_mode) + 1u) % 9u);
        y += row_height;
        section();
        toggle(controls.directional_light);
        toggle(controls.environment_lighting);
        toggle(controls.fog);
        toggle(controls.day_night_cycle);
        section();
        toggle(controls.point_lights);
        toggle(controls.point_shadows);
        toggle(controls.spot_light);
        toggle(controls.projected_texture);
        section();
        toggle(controls.normal_mapping);
        toggle(controls.parallax);
        toggle(controls.clearcoat);
        toggle(controls.reflection_refraction);
        toggle(controls.toon);
        toggle(controls.rim_lighting);
    } else if (selected_tab_ == 1) {
        section();
        toggle(controls.bloom);
        toggle(controls.fxaa);
        toggle(controls.shadows);
        section();
        toggle(controls.water_simulation);
        toggle(controls.cloth_simulation);
        toggle(controls.billboards);
        toggle(controls.particles);
        toggle(controls.animation);
        section();
        toggle(controls.instancing);
        toggle(controls.render_to_texture);
        toggle(controls.mirror_rtt);
        section();
        toggle(controls.show_labels);
        toggle(controls.show_help);
    } else if (selected_tab_ == 2) {
        section();
        advanced_toggle(controls.ssao);
        advanced_toggle(controls.tessellation);
        advanced_toggle(controls.pn_triangles);
        advanced_toggle(controls.indirect_draw);
        advanced_toggle(controls.gpu_queries);
        section();
        toggle(controls.tier0_mobile_profile);
        if (controls.tier0_mobile_profile) disable_tier1(controls);
    } else if (selected_tab_ == 3) {
        section();
        scalar(controls.gui_scale, 1.0f, 5.0f, 0.1f, 0);
        scalar(controls.target_fps, 30.0f, 500.0f, 1.0f, 1);
        scalar(controls.exposure, 0.25f, 3.0f, 0.05f, 2);
        scalar(controls.gamma, 0.8f, 4.0f, 0.05f, 24);
        scalar(controls.bloom_strength, 0.0f, 1.5f, 0.05f, 3);
        scalar(controls.bloom_threshold, 0.35f, 3.0f, 0.05f, 4);
        section();
        scalar(controls.sun_intensity, 0.0f, 2.5f, 0.05f, 5);
        scalar(controls.environment_strength, 0.0f, 1.5f, 0.05f, 6);
        scalar(controls.fog_density, 0.0f, 0.05f, 0.001f, 7);
        scalar(controls.fog_height_falloff, 0.0f, 0.5f, 0.01f, 8);
        scalar(controls.projector_strength, 0.0f, 2.5f, 0.05f, 9);
        section();
        scalar(controls.normal_strength, 0.0f, 2.0f, 0.05f, 10);
        scalar(controls.parallax_strength, 0.0f, 2.0f, 0.05f, 11);
        scalar(controls.clearcoat_strength, 0.0f, 2.0f, 0.05f, 12);
        scalar(controls.transmission_strength, 0.0f, 2.0f, 0.05f, 13);
        section();
        scalar(controls.animation_speed, 0.0f, 4.0f, 0.05f, 14);
        scalar(controls.particle_strength, 0.0f, 4.0f, 0.05f, 25);
        scalar(controls.water_wave_strength, 0.0f, 3.0f, 0.05f, 15);
        scalar(controls.water_refraction_strength, 0.0f, 2.0f, 0.05f, 16);
        scalar(controls.cloth_wind_strength, 0.0f, 2.5f, 0.05f, 17);
        scalar(controls.foliage_density, 0.25f, 2.0f, 0.05f, 18);
        scalar(controls.day_night_speed, 0.05f, 6.0f, 0.05f, 19);
        section();
        scalar(controls.ssao_strength, 0.0f, 2.0f, 0.05f, 20);
        scalar(controls.ssao_radius, 0.25f, 3.0f, 0.05f, 21);
        scalar(controls.tessellation_level, 1.0f, 32.0f, 1.0f, 22);
        section();
        scalar(controls.bench_explode, 0.0f, 2.0f, 0.05f, 23);
    } else {
        section();
        toggle(controls.tier0_mobile_profile);
        if (controls.tier0_mobile_profile) disable_tier1(controls);
        toggle(controls.wireframe);
        toggle(controls.frame_limit);
        toggle(controls.vsync);
        section();
        toggle(controls.scene_debug_view);
        toggle(controls.debug_hidden_objects);
        toggle(controls.debug_effect_bounds);
        section();
        toggle(controls.show_gui);
        toggle(controls.show_labels);
        toggle(controls.show_help);
        toggle(controls.day_night_cycle);
    }

    if (selected_object_ != invalid_editor_object && selected_object_ < scene.objects.size()) {
        update_inspector(input, frame, scene, mouse, window_input);
        captures_mouse_ = captures_mouse_ || inspector_layout_.hovered
            || inspector_.dragging || inspector_.resizing
            || active_numeric_scrub_ != invalid_editor_object;
    }

    if (input.left_mouse_released && !captures_mouse_)
        select_object_at(input, frame, controls, scene);

    clamp_controls(controls);
    return captures_mouse_;
}

void GuiOverlay::push_quad(float x, float y, float width, float height, math::Vec4 color,
                           math::Vec2 uv0, math::Vec2 uv1, float textured) {
    const Vertex a{{x, y}, uv0, color, textured};
    const Vertex b{{x + width, y}, {uv1.x, uv0.y}, color, textured};
    const Vertex c{{x, y + height}, {uv0.x, uv1.y}, color, textured};
    const Vertex d{{x + width, y + height}, uv1, color, textured};
    vertices_.insert(vertices_.end(), {a, b, d, a, d, c});
}

void GuiOverlay::screen_rectangle(egl::Rect rect, math::Vec4 color) {
    push_quad(rect.position.x, rect.position.y, rect.size.x, rect.size.y, color);
}

void GuiOverlay::rectangle(egl::Rect rect, math::Vec4 color) {
    screen_rectangle({
        {rect.position.x * ui_scale_, rect.position.y * ui_scale_},
        {rect.size.x * ui_scale_, rect.size.y * ui_scale_}
    }, color);
}

void GuiOverlay::screen_text(float x, float y, std::string_view value, math::Vec4 color, float scale) {
    const float glyph_width = base_glyph_width * scale;
    const float glyph_height = base_glyph_height * scale;
    const float glyph_advance = base_glyph_advance * scale;
    float cursor = x;
    for (const unsigned char character : value) {
        if (character == '\n') {
            cursor = x;
            y += glyph_height;
            continue;
        }
        if (character < 32 || character > 126) {
            cursor += glyph_advance;
            continue;
        }
        const int index = character - 32;
        const int column = index % 16;
        const int row = index / 16;
        constexpr float atlas_width = 512.0f;
        constexpr float atlas_height = 288.0f;
        constexpr float inset_u = 1.0f / atlas_width;
        constexpr float inset_v = 1.0f / atlas_height;
        const float u0 = static_cast<float>(column) / 16.0f + inset_u;
        const float u1 = static_cast<float>(column + 1) / 16.0f - inset_u;
        const float v0 = static_cast<float>(row) / 6.0f + inset_v;
        const float v1 = static_cast<float>(row + 1) / 6.0f - inset_v;
        push_quad(cursor + 1.25f * scale, y + 1.25f * scale, glyph_width, glyph_height,
                  {0.0f, 0.0f, 0.0f, color.w * 0.72f}, {u0, v0}, {u1, v1}, 1.0f);
        push_quad(cursor, y, glyph_width, glyph_height, color, {u0, v0}, {u1, v1}, 1.0f);
        cursor += glyph_advance;
    }
}

void GuiOverlay::text(float x, float y, std::string_view value, math::Vec4 color, float scale) {
    screen_text(x * ui_scale_, y * ui_scale_, value, color, scale * ui_scale_);
}

void GuiOverlay::label_value(float x, float y, std::string_view label, float value) {
    char buffer[112]{};
    std::snprintf(buffer, sizeof(buffer), "%.*s  %.3f",
                  static_cast<int>(label.size()), label.data(), value);
    text(x, y, buffer, text_color, 0.90f);
}

void GuiOverlay::build_panel(const context::FrameContext& frame,
                             const context::RuntimeControls& controls,
                             const render::RenderCapabilities& capabilities) {
    rectangle(panel_layout_.window, panel_color);
    rectangle(panel_layout_.title_bar, title_color);
    text(panel_layout_.title_bar.position.x + 10.0f, panel_layout_.title_bar.position.y + 4.0f,
         "EpochGui OpenGL Scene Controls", text_color, 0.98f);
    rectangle(panel_layout_.close_button,
              panel_layout_.close_hovered ? math::Vec4{0.8f, 0.18f, 0.18f, 1.0f} : control_color);
    text(panel_layout_.close_button.position.x + 8.0f, panel_layout_.close_button.position.y + 1.0f,
         "x", text_color, 0.96f);

    float x = panel_layout_.content.position.x;
    float y = panel_layout_.content.position.y;
    const float width = panel_layout_.content.size.x;
    text(x, y, "Grouped by portable baseline and optional desktop tiers", muted_color, 0.74f);
    y += 34.0f;

    const float tab_width = std::max(44.0f, (width - 8.0f) / static_cast<float>(tab_count));
    const std::array<float, tab_count> widths{tab_width, tab_width, tab_width, tab_width, tab_width};
    egl::SegmentedControlLayoutOptions tabs{{x, y}, widths.data(), static_cast<std::uint32_t>(tab_count), 25.0f, 2.0f};
    for (std::uint32_t index = 0; index < tab_count; ++index) {
        const auto rect = egl::segmented_control_item_layout(tabs, index);
        rectangle(rect, index == selected_tab_ ? accent_color : control_color);
        text(rect.position.x + 7.0f, rect.position.y + 3.0f, tab_names[index], text_color, 0.72f);
    }
    y += 38.0f;

    const float clip_top = y;
    const float clip_bottom = panel_layout_.window.position.y + panel_layout_.window.size.y - 34.0f;
    y -= tab_scroll_[selected_tab_];

    const auto visible = [&](float top, float height) noexcept {
        return top + height >= clip_top && top <= clip_bottom;
    };
    const auto section = [&](std::string_view label) {
        if (visible(y, section_height)) {
            rectangle({{x, y + 2.0f}, {width, section_height - 4.0f}}, section_color);
            text(x + 7.0f, y + 2.0f, label, accent_color, 0.74f);
        }
        y += section_height;
    };
    const auto row = [&](std::string_view label, bool value) {
        if (visible(y, row_height)) {
            const egl::Rect rect{{x, y}, {width, row_height}};
            rectangle(rect, control_color);
            text(x + 7.0f, y + 3.0f, label, text_color, 0.76f);
            const egl::Rect track{{x + width - 38.0f, y + (row_height - 18.0f) * 0.5f}, {34.0f, 18.0f}};
            rectangle(track, value ? accent_color : hover_color);
            const egl::Rect thumb{{track.position.x + (value ? 17.0f : 1.0f), track.position.y + 1.0f}, {16.0f, 16.0f}};
            rectangle(thumb, {0.88f, 0.92f, 0.97f, 1.0f});
        }
        y += row_height;
    };
    const auto cycle = [&](std::string_view label, std::string_view value) {
        if (visible(y, row_height)) {
            const egl::Rect rect{{x, y}, {width, row_height}};
            rectangle(rect, control_color);
            text(x + 7.0f, y + 3.0f, label, text_color, 0.76f);
            const float value_x = x + width - static_cast<float>(value.size()) * 9.2f - 12.0f;
            text(value_x, y + 3.0f, value, accent_color, 0.72f);
        }
        y += row_height;
    };
    const auto scalar = [&](std::string_view label, float value, std::size_t index) {
        if (visible(y, numeric_row_height)) {
            text(x + 7.0f, y + 5.0f, label, text_color, 0.66f);
            const float controls_x = x + width - 190.0f;
            const egl::Rect minus_rect{{controls_x, y + 2.0f}, {28.0f, 24.0f}};
            const egl::Rect value_rect{{controls_x + 34.0f, y + 2.0f}, {122.0f, 24.0f}};
            const egl::Rect plus_rect{{controls_x + 162.0f, y + 2.0f}, {28.0f, 24.0f}};
            rectangle(minus_rect, control_color);
            rectangle(plus_rect, control_color);
            text(minus_rect.position.x + 9.0f, minus_rect.position.y + 1.0f, "-", text_color, 0.76f);
            text(plus_rect.position.x + 8.0f, plus_rect.position.y + 1.0f, "+", text_color, 0.76f);

            const std::size_t edit_id = tuning_edit_base + index;
            const bool numeric_active = active_numeric_edit_ == edit_id || active_numeric_scrub_ == edit_id;
            if (numeric_active)
                rectangle({{value_rect.position.x - 1.0f, value_rect.position.y - 1.0f},
                           {value_rect.size.x + 2.0f, value_rect.size.y + 2.0f}}, accent_color);
            rectangle(value_rect, section_color);
            char value_text[48]{};
            std::snprintf(value_text, sizeof(value_text), "%.7g", value);
            std::string shown = active_numeric_edit_ == edit_id ? numeric_edit_buffer_ : std::string(value_text);
            if (active_numeric_edit_ == edit_id) shown += "|";
            if (shown.size() > 16u) shown.resize(16u);
            text(value_rect.position.x + 5.0f, value_rect.position.y + 1.0f, shown,
                 numeric_active ? text_color : accent_color, 0.62f);
        }
        y += numeric_row_height;
    };

    if (selected_tab_ == 0) {
        cycle("Time and weather", scene_name(controls.scene_preset));
        cycle("Shading/debug view", shading_name(controls.shading_mode));
        section("Environment");
        row("Directional sunlight", controls.directional_light);
        row("Cubemap ambient and reflections", controls.environment_lighting);
        row("Distance and height fog", controls.fog);
        row("Animated day/night cycle", controls.day_night_cycle);
        section("Local lighting");
        row("Local point lights", controls.point_lights);
        row("Point-light cubemap shadow", controls.point_shadows);
        row("Projected spotlight", controls.spot_light);
        row("Projected texture cookie", controls.projected_texture);
        section("Material routing");
        row("Normal mapping", controls.normal_mapping);
        row("Parallax occlusion mapping", controls.parallax);
        row("Clearcoat response", controls.clearcoat);
        row("Transmission and refraction", controls.reflection_refraction);
        row("Toon-routed materials", controls.toon);
        row("Rim-routed materials", controls.rim_lighting);
    } else if (selected_tab_ == 1) {
        section("Portable post processing");
        row("HDR bloom", controls.bloom);
        row("FXAA", controls.fxaa);
        row("Directional PCF shadows", controls.shadows);
        section("Portable simulations");
        row("Depth-aware water surfaces", controls.water_simulation);
        row("Constraint cloth and flag", controls.cloth_simulation);
        row("Instanced vertex foliage", controls.billboards);
        row("GPU particle fire", controls.particles);
        row("Scene animation", controls.animation);
        section("Portable geometry and RTT");
        row("Instanced prop batches", controls.instancing);
        row("Live camera render targets", controls.render_to_texture);
        row("One-sided planar mirror RTT", controls.mirror_rtt);
        section("Presentation");
        row("World-space system labels", controls.show_labels);
        row("Keyboard help bar", controls.show_help);
    } else if (selected_tab_ == 2) {
        section("Desktop optional paths");
        row("Screen-space ambient occlusion", controls.ssao);
        row("Hardware terrain tessellation", controls.tessellation);
        row("PN-style patch curvature", controls.pn_triangles);
        row("Indirect prop submission", controls.indirect_draw);
        row("GPU timing queries", controls.gpu_queries);
        section("Compatibility profile");
        row("Tier-0 mobile and Switch profile", controls.tier0_mobile_profile);
        if (visible(y, 28.0f))
            text(x + 7.0f, y + 3.0f, "Enabling a Tier-1 feature automatically leaves Tier 0.", muted_color, 0.62f);
        y += 28.0f;
    } else if (selected_tab_ == 3) {
        section("Display");
        scalar("GUI scale", controls.gui_scale, 0);
        scalar("Frame-limit target", controls.target_fps, 1);
        scalar("Exposure", controls.exposure, 2);
        scalar("Gamma", controls.gamma, 24);
        scalar("Bloom strength", controls.bloom_strength, 3);
        scalar("Bloom threshold", controls.bloom_threshold, 4);
        section("Lighting and atmosphere");
        scalar("Sun intensity", controls.sun_intensity, 5);
        scalar("Environment strength", controls.environment_strength, 6);
        scalar("Fog density", controls.fog_density, 7);
        scalar("Fog height falloff", controls.fog_height_falloff, 8);
        scalar("Projector intensity", controls.projector_strength, 9);
        section("Materials");
        scalar("Normal-map strength", controls.normal_strength, 10);
        scalar("Parallax depth", controls.parallax_strength, 11);
        scalar("Clearcoat strength", controls.clearcoat_strength, 12);
        scalar("Transmission strength", controls.transmission_strength, 13);
        section("Simulation and world");
        scalar("Animation speed", controls.animation_speed, 14);
        scalar("Particle strength", controls.particle_strength, 25);
        scalar("Water wave strength", controls.water_wave_strength, 15);
        scalar("Water refraction", controls.water_refraction_strength, 16);
        scalar("Cloth wind", controls.cloth_wind_strength, 17);
        scalar("Foliage density", controls.foliage_density, 18);
        scalar("Day/night cycles per minute", controls.day_night_speed, 19);
        section("Tier-1 tuning");
        scalar("SSAO strength", controls.ssao_strength, 20);
        scalar("SSAO radius", controls.ssao_radius, 21);
        scalar("Tessellation level", controls.tessellation_level, 22);
        section("Demonstration controls");
        scalar("Bench exploded view", controls.bench_explode, 23);
    } else {
        section("Compatibility");
        row("Tier-0 mobile and Switch profile", controls.tier0_mobile_profile);
        row("Wireframe", controls.wireframe);
        row("Frame limiter", controls.frame_limit);
        row("VSync", controls.vsync);
        section("Scene editor debug");
        row("Debug / x-ray view [F7]", controls.scene_debug_view);
        row("Include disabled objects", controls.debug_hidden_objects);
        row("Include effect proxy bounds", controls.debug_effect_bounds);
        section("Overlay");
        row("Control window", controls.show_gui);
        row("World labels", controls.show_labels);
        row("Help bar", controls.show_help);
        row("Day/night cycle", controls.day_night_cycle);
    }

    const float footer_y = panel_layout_.window.position.y + panel_layout_.window.size.y - 24.0f;
    rectangle({{panel_layout_.window.position.x + 4.0f, footer_y - 2.0f},
               {panel_layout_.window.size.x - 8.0f, 22.0f}}, title_color);
    char footer[144]{};
    std::snprintf(footer, sizeof(footer), "GL %d.%d | %s | %.0fx AF | wheel scroll",
                  capabilities.gl_major, capabilities.gl_minor,
                  controls.tier0_mobile_profile ? "Tier 0" : "Tier 1",
                  capabilities.max_anisotropy);
    text(panel_layout_.window.position.x + 10.0f, footer_y + 1.0f, footer, muted_color, 0.58f);

    const float max_scroll = std::max(0.0f, tab_content_height(selected_tab_) -
        std::max(30.0f, clip_bottom - clip_top));
    if (max_scroll > 0.0f) {
        const float fraction = std::clamp(tab_scroll_[selected_tab_] / max_scroll, 0.0f, 1.0f);
        const float track_height = std::max(26.0f, clip_bottom - clip_top);
        rectangle({{panel_layout_.window.position.x + panel_layout_.window.size.x - 7.0f, clip_top},
                   {3.0f, track_height}}, hover_color);
        rectangle({{panel_layout_.window.position.x + panel_layout_.window.size.x - 8.0f,
                    clip_top + fraction * std::max(0.0f, track_height - 36.0f)},
                   {5.0f, 36.0f}}, accent_color);
    }

    const float logical_height = static_cast<float>(frame.framebuffer_height) / ui_scale_;
    if (controls.scene_debug_view) {
        const float legend_y = logical_height - (controls.show_help ? 51.0f : 27.0f);
        rectangle({{8.0f, legend_y}, {1030.0f, 21.0f}}, {0.02f, 0.028f, 0.042f, 0.90f});
        text(14.0f, legend_y + 3.0f,
             "DEBUG: disabled pink | point yellow | spot orange | cloth cyan | water blue | RTT purple | foliage green | particles red",
             text_color, 0.46f);
    }

    if (controls.show_help) {
        const float help_y = logical_height - 27.0f;
        const float help_width = inspector_.open && selected_object_ != invalid_editor_object
            ? std::max(360.0f, inspector_layout_.window.position.x - 16.0f)
            : 850.0f;
        rectangle({{8.0f, help_y}, {help_width, 21.0f}}, {0.02f, 0.028f, 0.042f, 0.86f});
        text(14.0f, help_y + 3.0f,
             "LMB select | Ctrl/Shift add | Alt effects | empty click deselect | RMB look | WASD/QE move",
             text_color, 0.48f);
    }
}

void GuiOverlay::build_selection_marker(const context::FrameContext& frame,
                                        const context::RuntimeControls& controls,
                                        const render::scene::SceneSpine& scene) {
    const auto indices = selected_indices(scene);
    if (indices.empty() || frame.framebuffer_width <= 0 || frame.framebuffer_height <= 0) return;
    const float aspect = static_cast<float>(frame.framebuffer_width)
        / static_cast<float>(frame.framebuffer_height);
    const math::Mat4 view_projection = scene.camera.projection(aspect) * scene.camera.view();

    for (const auto index : indices) {
        if (index >= scene.objects.size()) continue;
        const auto& object = scene.objects[index];
        if (object.editor_deleted) continue;
        const bool debug_visible = controls.scene_debug_view
            && ((object.editor_only && controls.debug_effect_bounds)
                || (!object.visible && controls.debug_hidden_objects));
        if (!object.visible && !debug_visible) continue;
        const math::Vec3 pick_position = object.transform.position + object.editor_pick_offset;
        const math::Vec4 clip = view_projection * math::Vec4{
            pick_position.x, pick_position.y, pick_position.z, 1.0f
        };
        if (clip.w <= 0.05f) continue;
        const float inverse_w = 1.0f / clip.w;
        const float ndc_x = clip.x * inverse_w;
        const float ndc_y = clip.y * inverse_w;
        const float ndc_z = clip.z * inverse_w;
        if (ndc_z < -1.0f || ndc_z > 1.0f || std::abs(ndc_x) > 1.1f || std::abs(ndc_y) > 1.1f) continue;

        const float screen_x = (ndc_x * 0.5f + 0.5f) * static_cast<float>(frame.framebuffer_width);
        const float screen_y = (1.0f - (ndc_y * 0.5f + 0.5f)) * static_cast<float>(frame.framebuffer_height);
        const float distance = math::length(pick_position - scene.camera.position);
        const float extent = std::max({std::abs(object.transform.scale.x),
                                       std::abs(object.transform.scale.y),
                                       std::abs(object.transform.scale.z)})
            * std::max(object.editor_pick_radius_scale, 0.1f);
        const float radius = std::clamp(extent * 240.0f / std::max(distance, 0.5f), 18.0f, 92.0f);
        constexpr float thickness = 2.0f;
        constexpr float corner = 12.0f;
        const bool primary = index == selected_object_;
        const math::Vec4 marker = primary
            ? math::Vec4{0.16f, 0.72f, 1.0f, 0.96f}
            : math::Vec4{0.32f, 0.92f, 0.62f, 0.86f};
        screen_rectangle({{screen_x - radius, screen_y - radius}, {corner, thickness}}, marker);
        screen_rectangle({{screen_x - radius, screen_y - radius}, {thickness, corner}}, marker);
        screen_rectangle({{screen_x + radius - corner, screen_y - radius}, {corner, thickness}}, marker);
        screen_rectangle({{screen_x + radius - thickness, screen_y - radius}, {thickness, corner}}, marker);
        screen_rectangle({{screen_x - radius, screen_y + radius - thickness}, {corner, thickness}}, marker);
        screen_rectangle({{screen_x - radius, screen_y + radius - corner}, {thickness, corner}}, marker);
        screen_rectangle({{screen_x + radius - corner, screen_y + radius - thickness}, {corner, thickness}}, marker);
        screen_rectangle({{screen_x + radius - thickness, screen_y + radius - corner}, {thickness, corner}}, marker);
    }
}

void GuiOverlay::build_scene_labels(const context::FrameContext& frame,
                                    const context::RuntimeControls& controls,
                                    const render::scene::SceneSpine& scene) {
    if (!controls.show_labels || frame.framebuffer_width <= 0 || frame.framebuffer_height <= 0) return;

    const float aspect = static_cast<float>(frame.framebuffer_width) / static_cast<float>(frame.framebuffer_height);
    const math::Mat4 view_projection = scene.camera.projection(aspect) * scene.camera.view();
    std::vector<egl::Rect> placed_labels;
    placed_labels.reserve(scene.labels.size());

    const auto intersects = [](const egl::Rect& a, const egl::Rect& b) noexcept {
        return a.position.x < b.position.x + b.size.x
            && a.position.x + a.size.x > b.position.x
            && a.position.y < b.position.y + b.size.y
            && a.position.y + a.size.y > b.position.y;
    };

    const egl::Rect panel_screen{
        {panel_layout_.window.position.x * ui_scale_, panel_layout_.window.position.y * ui_scale_},
        {panel_layout_.window.size.x * ui_scale_, panel_layout_.window.size.y * ui_scale_}
    };
    const egl::Rect inspector_screen{
        {inspector_layout_.window.position.x * ui_scale_, inspector_layout_.window.position.y * ui_scale_},
        {inspector_layout_.window.size.x * ui_scale_, inspector_layout_.window.size.y * ui_scale_}
    };
    constexpr std::size_t maximum_visible_labels = 12u;

    for (const auto& label : scene.labels) {
        if (placed_labels.size() >= maximum_visible_labels) break;
        if (!label.visible) continue;
        const math::Vec3 delta = label.world_position - scene.camera.position;
        const float distance = math::length(delta);
        if (distance > label.max_distance) continue;

        // The large structural rear wall is opaque. Labels behind it must not render
        // as HUD text through the building. Intersect the camera-to-label segment
        // with the wall's front plane and reject hits inside its authored bounds.
        constexpr float rear_wall_z = -19.29f;
        constexpr float rear_wall_half_width = 15.20f;
        constexpr float rear_wall_min_y = 0.0f;
        constexpr float rear_wall_max_y = 6.72f;
        if (std::abs(delta.z) > 0.0001f) {
            const float wall_t = (rear_wall_z - scene.camera.position.z) / delta.z;
            if (wall_t > 0.0f && wall_t < 1.0f) {
                const math::Vec3 wall_hit = scene.camera.position + delta * wall_t;
                if (std::abs(wall_hit.x) <= rear_wall_half_width
                    && wall_hit.y >= rear_wall_min_y
                    && wall_hit.y <= rear_wall_max_y)
                    continue;
            }
        }

        const math::Vec4 clip = view_projection * math::Vec4{
            label.world_position.x, label.world_position.y, label.world_position.z, 1.0f
        };
        if (clip.w <= 0.05f) continue;
        const float inverse_w = 1.0f / clip.w;
        const float ndc_x = clip.x * inverse_w;
        const float ndc_y = clip.y * inverse_w;
        const float ndc_z = clip.z * inverse_w;
        if (ndc_z < -1.0f || ndc_z > 1.0f || std::abs(ndc_x) > 1.08f || std::abs(ndc_y) > 1.08f) continue;

        float screen_x = (ndc_x * 0.5f + 0.5f) * static_cast<float>(frame.framebuffer_width);
        float screen_y = (1.0f - (ndc_y * 0.5f + 0.5f)) * static_cast<float>(frame.framebuffer_height);
        const float fade = std::clamp(1.0f - distance / label.max_distance, 0.26f, 1.0f);
        const float scale = std::clamp(0.86f - distance * 0.0048f, 0.64f, 0.82f);
        std::size_t line_count = 1;
        std::size_t current_line = 0;
        std::size_t max_line = 0;
        for (const char character : label.text) {
            if (character == '\n') {
                max_line = std::max(max_line, current_line);
                current_line = 0;
                ++line_count;
            } else {
                ++current_line;
            }
        }
        max_line = std::max(max_line, current_line);
        const float width = static_cast<float>(max_line) * base_glyph_advance * scale + 20.0f;
        const float height = static_cast<float>(line_count) * base_glyph_height * scale + 12.0f;
        screen_x = std::clamp(screen_x - width * 0.5f, 6.0f,
                              static_cast<float>(frame.framebuffer_width) - width - 6.0f);
        const float preferred_y = std::clamp(screen_y - height - 10.0f, 6.0f,
                              static_cast<float>(frame.framebuffer_height) - height - 6.0f);

        bool placed = false;
        egl::Rect background{};
        for (int attempt = 0; attempt < 9 && !placed; ++attempt) {
            const int band = (attempt + 1) / 2;
            const float direction = attempt == 0 ? 0.0f : (attempt % 2 == 1 ? -1.0f : 1.0f);
            const float candidate_y = std::clamp(
                preferred_y + direction * static_cast<float>(band) * (height + 6.0f),
                6.0f,
                static_cast<float>(frame.framebuffer_height) - height - 6.0f);
            const egl::Rect candidate{{screen_x, candidate_y}, {width, height}};
            const bool under_panel = controls.show_gui && panel_.open && intersects(candidate, panel_screen);
            const bool under_inspector = controls.show_gui && inspector_.open
                && selected_object_ != invalid_editor_object && intersects(candidate, inspector_screen);
            const bool collides = std::ranges::any_of(placed_labels, [&](const egl::Rect& other) {
                return intersects(candidate, other);
            });
            if (!under_panel && !under_inspector && !collides) {
                background = candidate;
                placed = true;
            }
        }
        if (!placed) continue;
        placed_labels.push_back(background);

        screen_rectangle(background, {0.018f, 0.026f, 0.040f, 0.84f * fade});
        screen_rectangle({{background.position.x, background.position.y + height - 2.0f}, {width, 2.0f}},
                         with_alpha(label.color, fade));
        screen_text(background.position.x + 8.0f, background.position.y + 4.0f,
                    label.text, with_alpha(label.color, fade), scale);
    }
}

void GuiOverlay::build_inspector(const render::scene::SceneSpine& scene) {
    if (selected_object_ == invalid_editor_object || selected_object_ >= scene.objects.size()
        || !inspector_.open || !inspector_layout_.visible) return;

    rectangle(inspector_layout_.window, panel_color);
    rectangle(inspector_layout_.title_bar, title_color);
    text(inspector_layout_.title_bar.position.x + 10.0f,
         inspector_layout_.title_bar.position.y + 3.0f,
         "Scene Object Inspector", text_color, 0.84f);

    const auto& selected = scene.objects[selected_object_];
    float x = inspector_layout_.content.position.x;
    float y = inspector_layout_.content.position.y;
    const float width = inspector_layout_.content.size.x;

    std::string object_name = manual_multiselect_
        ? std::to_string(multi_selected_objects_.size()) + " entities selected"
        : selected.name;
    if (object_name.size() > 42u) object_name.resize(42u);
    text(x, y, object_name, accent_color, 0.76f);
    y += 20.0f;
    if (manual_multiselect_) {
        text(x, y, "Ctrl/Shift-click toggles selection", muted_color, 0.66f);
    } else if (!selected.editor_group.empty()) {
        std::string group = "Group: " + selected.editor_group;
        if (group.size() > 48u) group.resize(48u);
        text(x, y, group, muted_color, 0.66f);
    } else {
        text(x, y, selected.editor_only ? "Alt-click prioritizes scene effects" : "Single object", muted_color, 0.66f);
    }
    y += 34.0f;

    const auto draw_button = [&](egl::Rect rect, std::string_view label) {
        rectangle(rect, control_color);
        text(rect.position.x + 8.0f, rect.position.y + 3.0f, label, text_color, 0.68f);
    };
    draw_button({{x, y}, {86.0f, 28.0f}}, "Previous");
    draw_button({{x + 92.0f, y}, {86.0f, 28.0f}}, "Next");
    const bool has_group = !selected.editor_group.empty();
    if (manual_multiselect_) {
        draw_button({{x + 184.0f, y}, {width - 184.0f, 28.0f}}, "Multi-select active");
    } else if (has_group) {
        draw_button({{x + 184.0f, y}, {width - 184.0f, 28.0f}}, editor_group_mode_ ? "Selection: Group" : "Selection: Single");
    }
    y += 34.0f;

    rectangle({{x, y}, {width, 30.0f}}, control_color);
    text(x + 7.0f, y + 3.0f, "Enabled", text_color, 0.72f);
    const egl::Rect visibility_track{{x + width - 38.0f, y + 6.0f}, {34.0f, 18.0f}};
    const auto active_indices = selected_indices(scene);
    const bool all_enabled = std::ranges::all_of(active_indices, [&](std::size_t index) {
        return index < scene.objects.size() && scene.objects[index].visible
            && !scene.objects[index].editor_deleted;
    });
    rectangle(visibility_track, all_enabled ? accent_color : hover_color);
    rectangle({{visibility_track.position.x + (all_enabled ? 17.0f : 1.0f),
                visibility_track.position.y + 1.0f}, {16.0f, 16.0f}},
              {0.88f, 0.92f, 0.97f, 1.0f});
    y += 42.0f;

    const float page_half = (width - 6.0f) * 0.5f;
    draw_button({{x, y}, {page_half, 28.0f}}, inspector_page_ == 0u ? "[Transform]" : "Transform");
    draw_button({{x + page_half + 6.0f, y}, {page_half, 28.0f}}, inspector_page_ == 1u ? "[Asset / Material]" : "Asset / Material");
    y += 34.0f;

    const auto draw_scalar = [&](std::string_view label, float value, float low, float high,
                                 std::size_t edit_id) {
        (void)low;
        (void)high;
        char value_text[48]{};
        std::snprintf(value_text, sizeof(value_text), "%.7g", value);
        text(x, y + 1.0f, label, text_color, 0.66f);

        const egl::Rect minus_rect{{x + 78.0f, y}, {28.0f, 24.0f}};
        const egl::Rect value_rect{{x + 112.0f, y}, {112.0f, 24.0f}};
        const egl::Rect plus_rect{{x + 230.0f, y}, {28.0f, 24.0f}};
        rectangle(minus_rect, control_color);
        rectangle(plus_rect, control_color);
        text(minus_rect.position.x + 9.0f, minus_rect.position.y + 1.0f, "-", text_color, 0.76f);
        text(plus_rect.position.x + 8.0f, plus_rect.position.y + 1.0f, "+", text_color, 0.76f);

        const bool numeric_active = active_numeric_edit_ == edit_id || active_numeric_scrub_ == edit_id;
        if (numeric_active)
            rectangle({{value_rect.position.x - 1.0f, value_rect.position.y - 1.0f},
                       {value_rect.size.x + 2.0f, value_rect.size.y + 2.0f}}, accent_color);
        rectangle(value_rect, section_color);
        std::string shown = active_numeric_edit_ == edit_id ? numeric_edit_buffer_ : std::string(value_text);
        if (active_numeric_edit_ == edit_id) shown += "|";
        if (shown.size() > 16u) shown.resize(16u);
        text(value_rect.position.x + 5.0f, value_rect.position.y + 1.0f, shown,
             numeric_active ? text_color : accent_color, 0.62f);
        y += 29.0f;
    };

    if (inspector_page_ == 0u) {
    draw_scalar("Pos X", editor_position_.x, selected.editor_bounds_min.x, selected.editor_bounds_max.x, 0u);
    draw_scalar("Pos Y", editor_position_.y, selected.editor_bounds_min.y, selected.editor_bounds_max.y, 1u);
    draw_scalar("Pos Z", editor_position_.z, selected.editor_bounds_min.z, selected.editor_bounds_max.z, 2u);

    using render::scene::EditorBindingKind;
    const auto binding = selected.editor_binding;
    const bool transform_rotation = binding == EditorBindingKind::none
        || binding == EditorBindingKind::water_surface
        || binding == EditorBindingKind::cloth_object
        || binding == EditorBindingKind::rtt_display
        || binding == EditorBindingKind::foliage_region;
    const bool transform_size = transform_rotation || binding == EditorBindingKind::terrain_surface;

    if (transform_rotation) {
        draw_scalar("Rot X", editor_rotation_degrees_.x, -180.0f, 180.0f, 3u);
        draw_scalar("Rot Y", editor_rotation_degrees_.y, -180.0f, 180.0f, 4u);
        draw_scalar("Rot Z", editor_rotation_degrees_.z, -180.0f, 180.0f, 5u);
    }
    if (transform_size && active_indices.size() == 1u) {
        draw_scalar("Size X", editor_size_.x, selected.editor_min_scale.x, selected.editor_max_scale.x, 6u);
        draw_scalar("Size Y", editor_size_.y, selected.editor_min_scale.y, selected.editor_max_scale.y, 7u);
        draw_scalar("Size Z", editor_size_.z, selected.editor_min_scale.z, selected.editor_max_scale.z, 8u);
    }
    if (transform_size) {
        draw_scalar("Scale X", editor_scale_.x, 0.01f, 50.0f, 9u);
        draw_scalar("Scale Y", editor_scale_.y, 0.01f, 50.0f, 10u);
        draw_scalar("Scale Z", editor_scale_.z, 0.01f, 50.0f, 11u);
    }

    if (active_indices.size() == 1u) {
        const auto properties = editor_properties(scene, selected);
        switch (selected.editor_binding) {
        case EditorBindingKind::water_surface:
            draw_scalar("Wave", properties[0], 0.0f, 0.5f, 12u);
            draw_scalar("Speed", properties[1], 0.0f, 4.0f, 13u);
            draw_scalar("Rough", properties[2], 0.0f, 1.0f, 14u);
            draw_scalar("Opacity", properties[3], 0.0f, 1.0f, 15u);
            break;
        case EditorBindingKind::cloth_object:
            draw_scalar("Wind", properties[0], 0.0f, 4.0f, 12u);
            draw_scalar("Gravity", properties[1], 0.0f, 2.0f, 13u);
            draw_scalar("Billow", properties[2], 0.0f, 1.0f, 14u);
            break;
        case EditorBindingKind::point_light:
            draw_scalar("Radius", properties[0], 0.25f, 50.0f, 12u);
            draw_scalar("Intensity", properties[1], 0.0f, 80.0f, 13u);
            break;
        case EditorBindingKind::spotlight:
            draw_scalar("Range", properties[0], 0.25f, 80.0f, 12u);
            draw_scalar("Intensity", properties[1], 0.0f, 100.0f, 13u);
            draw_scalar("Inner", properties[2], 0.0f, 1.0f, 14u);
            draw_scalar("Outer", properties[3], 0.0f, 1.0f, 15u);
            break;
        case EditorBindingKind::terrain_surface:
            draw_scalar("Height scale", properties[0], 0.0f, 24.0f, 12u);
            break;
        case EditorBindingKind::foliage_region:
            draw_scalar("Density", properties[0], 0.0f, 4.0f, 12u);
            draw_scalar("Blade W", properties[1], 0.0f, 4.0f, 13u);
            draw_scalar("Blade H", properties[2], 0.0f, 6.0f, 14u);
            draw_scalar("Sway", properties[3], 0.0f, 4.0f, 15u);
            break;
        case EditorBindingKind::particle_emitter:
            draw_scalar("Intensity", properties[0], 0.0f, 4.0f, 12u);
            draw_scalar("Particle size", properties[1], 0.0f, 6.0f, 13u);
            break;
        default:
            break;
        }
    }
    }

    if (inspector_page_ == 1u && !selected.editor_only) {
        rectangle({{x, y}, {width, 30.0f}}, section_color);
        std::string model_label = "Model: ";
        if (selected_model_option_ < scene.editor_models.size()) model_label += scene.editor_models[selected_model_option_].name;
        else model_label += "Custom / imported";
        if (model_label.size() > 50u) model_label.resize(50u);
        text(x + 7.0f, y + 3.0f, model_label, accent_color, 0.67f);
        y += 36.0f;

        const float half = (width - 6.0f) * 0.5f;
        draw_button({{x, y}, {half, 30.0f}}, "Replace model");
        draw_button({{x + half + 6.0f, y}, {half, 30.0f}}, "Load OBJ...");
        y += 38.0f;

        rectangle({{x, y}, {width, 30.0f}}, section_color);
        std::string material_label = "Material: ";
        if (selected_material_option_ < scene.materials.size()) material_label += scene.materials[selected_material_option_].name;
        else material_label += "None";
        if (material_label.size() > 50u) material_label.resize(50u);
        text(x + 7.0f, y + 3.0f, material_label, accent_color, 0.67f);
        y += 36.0f;

        rectangle({{x, y}, {width, 30.0f}}, section_color);
        const char* slot = selected_texture_slot_ == EditorImportRequest::normal_texture ? "Normal"
            : (selected_texture_slot_ == EditorImportRequest::orm_texture ? "ORM" : "Albedo");
        text(x + 7.0f, y + 3.0f, std::string("Texture slot: ") + slot, accent_color, 0.67f);
        y += 36.0f;

        const float material_third = (width - 12.0f) / 3.0f;
        draw_button({{x, y}, {material_third, 30.0f}}, "Apply");
        draw_button({{x + material_third + 6.0f, y}, {material_third, 30.0f}}, "Load texture");
        draw_button({{x + (material_third + 6.0f) * 2.0f, y}, {material_third, 30.0f}}, "Reset slot");
        y += 38.0f;

        if (active_indices.size() == 1u && selected.material && selected.material.value <= scene.materials.size()) {
            const auto& material = scene.material(selected.material);
            draw_scalar("UV scale", material.uv_scale, 0.05f, 20.0f, 16u);
            draw_scalar("Metallic", material.metallic_factor, 0.0f, 1.0f, 17u);
            draw_scalar("Roughness", material.roughness_factor, 0.0f, 1.0f, 18u);
            draw_scalar("Normal", material.normal_scale, 0.0f, 4.0f, 19u);
        }
    }

    if (inspector_page_ == 1u
        && selected.editor_binding == render::scene::EditorBindingKind::foliage_region
        && selected.editor_binding_index < scene.foliage_regions.size()) {
        const auto& region = scene.foliage_regions[selected.editor_binding_index];
        const float third_mask = (width - 12.0f) / 3.0f;
        draw_button({{x, y}, {third_mask, 30.0f}}, grass_paint_add_ ? "[Paint grass]" : "Paint grass");
        draw_button({{x + third_mask + 6.0f, y}, {third_mask, 30.0f}}, !grass_paint_add_ ? "[Erase]" : "Erase");
        draw_button({{x + (third_mask + 6.0f) * 2.0f, y}, {third_mask, 30.0f}}, "Allow all");
        y += 38.0f;
        draw_scalar("Brush cells", grass_brush_radius_, 1.0f, 16.0f, 2020u);

        const float canvas_size = std::min(width, 300.0f);
        const float cell_size = canvas_size / static_cast<float>(render::scene::foliage_mask_resolution);
        rectangle({{x - 1.0f, y - 1.0f}, {canvas_size + 2.0f, canvas_size + 2.0f}}, muted_color);
        for (std::size_t screen_y = 0; screen_y < render::scene::foliage_mask_resolution; ++screen_y) {
            const std::size_t mask_y = render::scene::foliage_mask_resolution - 1u - screen_y;
            for (std::size_t cell_x = 0; cell_x < render::scene::foliage_mask_resolution; ++cell_x) {
                const bool allowed = region.placement_mask[mask_y * render::scene::foliage_mask_resolution + cell_x] >= 128u;
                const math::Vec4 cell_color = allowed
                    ? math::Vec4{0.10f, 0.34f, 0.17f, 1.0f}
                    : math::Vec4{0.075f, 0.085f, 0.105f, 1.0f};
                rectangle({{x + static_cast<float>(cell_x) * cell_size,
                            y + static_cast<float>(screen_y) * cell_size},
                           {cell_size + 0.15f, cell_size + 0.15f}}, cell_color);
            }
        }
        y += canvas_size + 5.0f;
        text(x, y, "LMB uses selected brush; RMB erases.", muted_color, 0.62f);
        y += 18.0f;
        text(x, y, "Solid footprints remain excluded after painting.", muted_color, 0.62f);
        y += 26.0f;
    }

    const bool can_duplicate = std::ranges::any_of(active_indices, [&](std::size_t index) {
        return index < scene.objects.size() && !scene.objects[index].editor_only
            && scene.objects[index].editor_binding == render::scene::EditorBindingKind::none;
    });
    const float third = (width - 12.0f) / 3.0f;
    draw_button({{x, y}, {third, 30.0f}}, can_duplicate ? "Duplicate" : "Duplicate N/A");
    draw_button({{x + third + 6.0f, y}, {third, 30.0f}}, "Delete");
    draw_button({{x + (third + 6.0f) * 2.0f, y}, {third, 30.0f}}, "Deselect");
    y += 36.0f;
    draw_button({{x, y}, {third, 30.0f}}, "Save default");
    draw_button({{x + third + 6.0f, y}, {third, 30.0f}}, "Reload");
    draw_button({{x + (third + 6.0f) * 2.0f, y}, {third, 30.0f}}, "Reset");
    y += 36.0f;
}

void GuiOverlay::build_reopen_button() {
    rectangle(reopen_button_, {0.035f, 0.055f, 0.082f, 0.92f});
    rectangle({{reopen_button_.position.x, reopen_button_.position.y + reopen_button_.size.y - 2.0f},
               {reopen_button_.size.x, 2.0f}}, accent_color);
    text(reopen_button_.position.x + 11.0f, reopen_button_.position.y + 7.0f,
         "Open controls [F3]", text_color, 0.80f);
}

void GuiOverlay::render(const context::FrameContext& frame,
                        const context::RuntimeControls& controls,
                        const render::RenderCapabilities& capabilities,
                        const render::scene::SceneSpine& scene) {
    ui_scale_ = std::clamp(controls.gui_scale, 1.0f, 5.0f);
    vertices_.clear();
    build_selection_marker(frame, controls, scene);
    build_scene_labels(frame, controls, scene);
    if (controls.show_gui && panel_.open) build_panel(frame, controls, capabilities);
    else build_reopen_button();
    build_inspector(scene);
    if (vertices_.empty()) return;

    gl::BindFramebuffer(gl::framebuffer, 0);
    gl::Viewport(0, 0, frame.framebuffer_width, frame.framebuffer_height);
    gl::Disable(GL_DEPTH_TEST);
    gl::Disable(GL_CULL_FACE);
    gl::Enable(GL_BLEND);
    gl::BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    shader_.bind();
    shader_.set("uViewport", math::Vec2{static_cast<float>(frame.framebuffer_width),
                                        static_cast<float>(frame.framebuffer_height)});
    shader_.set("uFont", 0);
    font_.bind(0);
    gl::BindVertexArray(vao_);
    gl::BindBuffer(gl::array_buffer, vbo_);
    gl::BufferData(gl::array_buffer, static_cast<gl::GLsizeiptr>(vertices_.size() * sizeof(Vertex)),
                   vertices_.data(), gl::dynamic_draw);
    gl::DrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices_.size()));
    gl::BindVertexArray(0);
    gl::Disable(GL_BLEND);
    gl::Enable(GL_CULL_FACE);
    gl::Enable(GL_DEPTH_TEST);
}

} // namespace epoch::gui
