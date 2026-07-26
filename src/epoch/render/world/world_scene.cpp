#include "epoch/render/world/world_scene.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>



#if defined(EPOCH_NO_MODULES)
#include "epoch/compat/epoch.core.math.hpp"
#else
import epoch.core.math;
#endif
#if defined(EPOCH_NO_MODULES)
#include "epoch/compat/epoch.render.scene.hpp"
#else
import epoch.render.scene;
#endif
namespace epoch::render::world {
namespace {
using scene::MaterialFeature;

constexpr std::uint32_t day_mask = scene::SceneSpine::preset_bit(context::ScenePreset::material_lab);
constexpr std::uint32_t dusk_mask = scene::SceneSpine::preset_bit(context::ScenePreset::lighting_lab);
constexpr std::uint32_t night_mask = scene::SceneSpine::preset_bit(context::ScenePreset::asset_gallery);
constexpr std::uint32_t diagnostics_mask = scene::SceneSpine::preset_bit(context::ScenePreset::diagnostics);
constexpr std::uint32_t storm_mask = scene::SceneSpine::preset_bit(context::ScenePreset::advanced_lab);
constexpr std::uint32_t world_mask = day_mask | dusk_mask | night_mask | diagnostics_mask | storm_mask;

struct HardcodedDefaultEntry {
    std::size_t index{};
    std::string_view name{};
    std::array<float, 3> position{};
    std::array<float, 3> rotation{};
    std::array<float, 3> size{};
    std::array<float, 3> relative_scale{};
    std::string_view model_name{};
    bool enabled{true};
    bool deleted{};
    std::array<float, 4> properties{};
    std::string_view editor_group{};
};

constexpr HardcodedDefaultEntry hardcoded_default_scene[]{
#include "hardcoded_scene_defaults.inc"
};

struct HardcodedMaterialEntry {
    std::size_t index{};
    std::string_view name{};
    std::string_view material_name{};
    std::string_view albedo_asset{};
    std::string_view normal_asset{};
    std::string_view orm_asset{};
    float uv_scale{1.0f};
    float metallic{1.0f};
    float roughness{1.0f};
    float normal_scale{1.0f};
};

constexpr HardcodedMaterialEntry hardcoded_material_scene[]{
#include "hardcoded_material_defaults.inc"
};

struct FactoryDeletedEntry {
    std::size_t original_index{};
    std::string_view name{};
};

constexpr FactoryDeletedEntry factory_deleted_objects[]{
    {87u, "North retaining face"},
    {90u, "East retaining face"},
};

void prune_factory_deleted_objects(scene::SceneSpine& scene) {
    constexpr std::size_t authored_count = sizeof(hardcoded_default_scene) / sizeof(hardcoded_default_scene[0]);
    constexpr std::size_t deleted_count = sizeof(factory_deleted_objects) / sizeof(factory_deleted_objects[0]);
    if (scene.objects.size() == authored_count) return;
    if (scene.objects.size() != authored_count + deleted_count)
        throw std::runtime_error("Constructed scene does not match the authored factory object count");

    for (auto iterator = std::rbegin(factory_deleted_objects);
         iterator != std::rend(factory_deleted_objects); ++iterator) {
        if (iterator->original_index >= scene.objects.size()
            || scene.objects[iterator->original_index].name != iterator->name)
            throw std::runtime_error("Factory-deleted scene object order/name mismatch");
        scene.objects.erase(scene.objects.begin() + static_cast<std::ptrdiff_t>(iterator->original_index));
    }
}

[[nodiscard]] MaterialHandle material_by_name(scene::SceneSpine& scene,
                                                     std::string_view name) noexcept {
    for (std::size_t index = 0; index < scene.materials.size(); ++index) {
        if (scene.materials[index].name == name)
            return MaterialHandle{static_cast<std::uint32_t>(index + 1u)};
    }
    return {};
}

void apply_hardcoded_material_state(ResourceSpine& resources, scene::SceneSpine& scene) {
    constexpr float epsilon = 1.0e-5f;
    for (const auto& entry : hardcoded_material_scene) {
        if (entry.index >= scene.objects.size()) continue;
        auto& object = scene.objects[entry.index];
        if (object.name != entry.name || object.editor_only) continue;

        const auto base_handle = material_by_name(scene, entry.material_name);
        if (!base_handle) continue;
        const auto& base = scene.material(base_handle);
        const bool customized = !entry.albedo_asset.empty() || !entry.normal_asset.empty() || !entry.orm_asset.empty()
            || std::abs(entry.uv_scale - base.uv_scale) > epsilon
            || std::abs(entry.metallic - base.metallic_factor) > epsilon
            || std::abs(entry.roughness - base.roughness_factor) > epsilon
            || std::abs(entry.normal_scale - base.normal_scale) > epsilon;
        if (!customized) {
            object.material = base_handle;
            continue;
        }

        auto material = base;
        material.name = "Authored: " + object.name;
        material.editor_unique = true;
        material.editor_base_material_name = base.name;
        material.editor_albedo_asset = entry.albedo_asset;
        material.editor_normal_asset = entry.normal_asset;
        material.editor_orm_asset = entry.orm_asset;
        material.uv_scale = entry.uv_scale;
        material.metallic_factor = entry.metallic;
        material.roughness_factor = entry.roughness;
        material.normal_scale = entry.normal_scale;
        const auto load_authored_texture = [&](std::string_view asset, gl::ColorSpace color_space,
                                               TextureHandle& destination) {
            if (asset.empty()) return;
            try {
                destination = resources.load_texture(asset, color_space);
            } catch (const std::exception&) {
                // Keep the base texture. The retained path allows a later editor reload to retry.
            }
        };
        load_authored_texture(entry.albedo_asset, gl::ColorSpace::srgb, material.albedo);
        load_authored_texture(entry.normal_asset, gl::ColorSpace::linear, material.normal);
        load_authored_texture(entry.orm_asset, gl::ColorSpace::linear, material.orm);
        object.material = scene.add_material(std::move(material));
    }
}

[[nodiscard]] scene::SpotLight& spotlight_at(scene::SceneSpine& scene, std::size_t index) noexcept {
    if (index == 0u) return scene.spotlight;
    if (index == 1u) return scene.secondary_spotlight;
    return scene.projector_spotlight;
}

void synchronize_hardcoded_editor_binding(scene::SceneSpine& scene, scene::RenderObject& object) {
    using scene::EditorBindingKind;
    const auto& properties = object.editor_properties;
    switch (object.editor_binding) {
    case EditorBindingKind::water_surface:
        if (object.editor_binding_index < scene.water_surfaces.size()) {
            auto& value = scene.water_surfaces[object.editor_binding_index];
            object.authored_size.y = 1.0f;
            object.relative_scale.y = 1.0f;
            object.transform.scale.y = 1.0f;
            value.transform = object.transform;
            value.wave_amplitude = std::clamp(properties[0], 0.0f, 0.5f);
            value.wave_speed = std::clamp(properties[1], 0.0f, 4.0f);
            value.roughness = std::clamp(properties[2], 0.0f, 1.0f);
            value.opacity = std::clamp(properties[3], 0.0f, 1.0f);
            value.visible = object.visible;
        }
        break;
    case EditorBindingKind::cloth_object:
        if (object.editor_binding_index < scene.cloth_objects.size()) {
            auto& value = scene.cloth_objects[object.editor_binding_index];
            value.origin = object.transform.position;
            value.width = std::max(object.transform.scale.x, 0.25f);
            value.height = std::max(object.transform.scale.y, 0.25f);
            value.wind_response = std::clamp(properties[0], 0.0f, 4.0f);
            value.gravity_scale = std::clamp(properties[1], 0.0f, 2.0f);
            value.initial_billow = std::clamp(properties[2], 0.0f, 1.0f);
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
            auto& value = scene.point_lights[object.editor_binding_index];
            value.position = object.transform.position;
            value.radius = std::clamp(properties[0], 0.25f, 50.0f);
            value.intensity = object.visible ? std::clamp(properties[1], 0.0f, 80.0f) : 0.0f;
        }
        break;
    case EditorBindingKind::spotlight: {
        auto& light = spotlight_at(scene, object.editor_binding_index);
        light.position = object.transform.position;
        light.range = std::clamp(properties[0], 0.25f, 80.0f);
        light.intensity = std::clamp(properties[1], 0.0f, 100.0f);
        light.inner_cosine = std::clamp(properties[2], 0.0f, 1.0f);
        light.outer_cosine = std::clamp(properties[3], 0.0f, light.inner_cosine);
        light.enabled = object.visible;
        break;
    }
    case EditorBindingKind::terrain_surface:
        object.transform.rotation.x = 0.0f;
        object.transform.rotation.z = 0.0f;
        scene.terrain.transform = object.transform;
        scene.terrain.height_scale = std::clamp(properties[0], 0.0f, 24.0f);
        scene.terrain.visible = object.visible;
        break;
    case EditorBindingKind::foliage_region:
        if (object.editor_binding_index < scene.foliage_regions.size()) {
            auto& value = scene.foliage_regions[object.editor_binding_index];
            object.authored_size.y = 1.0f;
            object.relative_scale.y = 1.0f;
            object.transform.scale.y = 1.0f;
            value.transform = object.transform;
            value.density = std::clamp(properties[0], 0.0f, 4.0f);
            value.blade_scale.x = std::clamp(properties[1], 0.1f, 4.0f);
            value.blade_scale.y = std::clamp(properties[2], 0.1f, 6.0f);
            value.sway = std::clamp(properties[3], 0.0f, 4.0f);
            value.visible = object.visible;
        }
        break;
    case EditorBindingKind::particle_emitter:
        if (object.editor_binding_index < scene.particle_emitters.size()) {
            auto& value = scene.particle_emitters[object.editor_binding_index];
            value.transform.position = object.transform.position;
            value.transform.rotation = object.transform.rotation;
            value.intensity = std::clamp(properties[0], 0.0f, 4.0f);
            value.size_scale = std::clamp(properties[1], 0.0f, 6.0f);
            value.visible = object.visible;
        }
        break;
    default:
        break;
    }
}

void apply_hardcoded_default_scene_state(scene::SceneSpine& scene) {
    prune_factory_deleted_objects(scene);
    constexpr std::size_t authored_count = sizeof(hardcoded_default_scene) / sizeof(hardcoded_default_scene[0]);
    if (scene.objects.size() != authored_count)
        throw std::runtime_error("Authored default scene object count does not match the constructed scene");

    for (const auto& entry : hardcoded_default_scene) {
        if (entry.index >= scene.objects.size())
            throw std::runtime_error("Authored default scene index is out of range");
        auto& object = scene.objects[entry.index];
        if (object.name != entry.name)
            throw std::runtime_error("Authored default scene object order/name mismatch");

        object.authored_position = {entry.position[0], entry.position[1], entry.position[2]};
        object.transform.position = object.authored_position;
        object.transform.rotation = {entry.rotation[0], entry.rotation[1], entry.rotation[2]};
        object.authored_size = {entry.size[0], entry.size[1], entry.size[2]};
        object.relative_scale = {entry.relative_scale[0], entry.relative_scale[1], entry.relative_scale[2]};
        object.transform.scale = object.authored_size * object.relative_scale;
        object.editor_properties = entry.properties;
        object.visible = entry.enabled && !entry.deleted;
        object.editor_deleted = entry.deleted;
        object.editor_group = std::string(entry.editor_group);

        if (!entry.model_name.empty()) {
            const auto model = std::find_if(scene.editor_models.begin(), scene.editor_models.end(),
                [&](const scene::EditorModelOption& option) { return option.name == entry.model_name; });
            if (model == scene.editor_models.end())
                throw std::runtime_error("Authored default scene references an unavailable editor model");
            object.mesh = model->mesh;
        }
        synchronize_hardcoded_editor_binding(scene, object);
    }
}

scene::RenderObject object(
    std::string name,
    MeshHandle mesh,
    MaterialHandle material,
    math::Vec3 position,
    math::Vec3 scale,
    math::Vec3 rotation = {},
    std::uint32_t mask = world_mask) {
    scene::RenderObject result;
    result.name = std::move(name);
    result.mesh = mesh;
    result.material = material;
    result.transform.position = position;
    result.authored_position = position;
    result.authored_size = scale;
    result.relative_scale = {1.0f, 1.0f, 1.0f};
    result.transform.scale = scale;
    result.transform.rotation = rotation;
    result.preset_mask = mask;
    result.visible = true;
    return result;
}

struct MaterialMaps {
    TextureHandle albedo{};
    TextureHandle normal{};
    TextureHandle orm{};
    TextureHandle height{};
    TextureHandle opacity{};
};

MaterialMaps load_maps(
    ResourceSpine& resources,
    const std::filesystem::path& root,
    TextureHandle default_opacity) {
    MaterialMaps maps;
    maps.albedo = resources.load_texture(root / "base_color.png", gl::ColorSpace::srgb);
    maps.normal = resources.load_texture(root / "normal.png", gl::ColorSpace::linear);
    maps.orm = resources.load_texture(root / "orm.png", gl::ColorSpace::linear);
    maps.height = resources.load_texture(root / "height.png", gl::ColorSpace::linear);
    const auto opacity_path = resources.asset_root() / root / "opacity.png";
    maps.opacity = std::filesystem::exists(opacity_path)
        ? resources.load_texture(root / "opacity.png", gl::ColorSpace::linear)
        : default_opacity;
    return maps;
}

MaterialMaps load_pack_maps(ResourceSpine& resources, std::string_view name, TextureHandle opacity) {
    return load_maps(resources, std::filesystem::path{"default_pack/textures/materials"} / name, opacity);
}

MaterialMaps load_curated_maps(ResourceSpine& resources, std::string_view name, TextureHandle opacity) {
    return load_maps(resources, std::filesystem::path{"curated/materials"} / name, opacity);
}

MaterialHandle add_material(
    scene::SceneSpine& scene,
    std::string name,
    const MaterialMaps& maps,
    TextureHandle emissive,
    math::Vec3 tint,
    float metallic,
    float roughness,
    float normal_scale,
    float height_scale,
    float uv_scale,
    MaterialFeature features,
    math::Vec3 emissive_factor = {},
    float clearcoat = 0.0f,
    float clearcoat_roughness = 0.12f,
    float transmission = 0.0f,
    float ior = 1.5f,
    float alpha_cutoff = 0.0f,
    bool unlit = false) {
    scene::Material material;
    material.name = std::move(name);
    material.albedo = maps.albedo;
    material.normal = maps.normal;
    material.orm = maps.orm;
    material.height = maps.height;
    material.opacity = maps.opacity;
    material.emissive = emissive;
    material.base_color = tint;
    material.emissive_factor = emissive_factor;
    material.metallic_factor = metallic;
    material.roughness_factor = roughness;
    material.normal_scale = normal_scale;
    material.height_scale = height_scale;
    (void)uv_scale;
    material.uv_scale = 1.0f;
    material.features = features;
    material.clearcoat_factor = clearcoat;
    material.clearcoat_roughness = clearcoat_roughness;
    material.transmission_factor = transmission;
    material.index_of_refraction = ior;
    material.alpha_cutoff = alpha_cutoff;
    material.unlit = unlit;
    return scene.add_material(std::move(material));
}

void add_wall_with_parallax_face(
    scene::SceneSpine& scene,
    MeshHandle cube,
    MeshHandle plane,
    MaterialHandle structure,
    MaterialHandle face,
    math::Vec3 center,
    float width,
    float height,
    float yaw = 0.0f) {
    auto backing = object("Brick wall structure", cube, structure,
        center, {width * 0.5f, height * 0.5f, 0.16f}, {0.0f, yaw, 0.0f});
    scene.objects.push_back(std::move(backing));

    const math::Vec3 facing{std::sin(yaw), 0.0f, std::cos(yaw)};
    auto receiver = object("Brick parallax receiver", plane, face,
        center + facing * 0.175f,
        {width * 0.5f, 1.0f, height * 0.5f},
        {math::radians(90.0f), yaw, 0.0f});
    receiver.casts_shadow = false;
    scene.objects.push_back(std::move(receiver));

    scene.objects.push_back(object("Wall cap", cube, structure,
        center + math::Vec3{0.0f, height * 0.5f + 0.08f, 0.0f},
        {width * 0.52f, 0.08f, 0.22f}, {0.0f, yaw, 0.0f}));
}

[[nodiscard]] math::Vec3 rotate_y_offset(math::Vec3 value, float yaw) noexcept {
    const float cosine = std::cos(yaw);
    const float sine = std::sin(yaw);
    return {
        cosine * value.x + sine * value.z,
        value.y,
        -sine * value.x + cosine * value.z
    };
}

[[nodiscard]] math::Vec3 direction_to_editor_rotation(math::Vec3 direction) noexcept {
    direction = math::normalize(direction);
    return {
        std::asin(std::clamp(direction.y, -1.0f, 1.0f)),
        std::atan2(-direction.x, -direction.z),
        0.0f
    };
}
} // namespace

void WorldSceneBuilder::apply_authored_defaults(ResourceSpine& resources, scene::SceneSpine& scene) {
    apply_hardcoded_default_scene_state(scene);
    apply_hardcoded_material_state(resources, scene);
}

bool WorldSceneBuilder::write_authored_default_config(
    const std::filesystem::path& path, bool overwrite) {
    std::error_code error;
    if (!overwrite && std::filesystem::exists(path, error)) return true;
    std::filesystem::create_directories(path.parent_path(), error);

    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output) return false;
    output << "EPOCH_SCENE_EDITOR_V5\n" << std::setprecision(9);
    for (const auto& entry : hardcoded_default_scene) {
        const auto material = std::find_if(std::begin(hardcoded_material_scene), std::end(hardcoded_material_scene),
            [&](const HardcodedMaterialEntry& candidate) {
                return candidate.index == entry.index && candidate.name == entry.name;
            });
        const bool has_material = material != std::end(hardcoded_material_scene);
        output << entry.index << ' ' << std::quoted(std::string(entry.name)) << ' '
               << entry.position[0] << ' ' << entry.position[1] << ' ' << entry.position[2] << ' '
               << entry.rotation[0] << ' ' << entry.rotation[1] << ' ' << entry.rotation[2] << ' '
               << entry.size[0] << ' ' << entry.size[1] << ' ' << entry.size[2] << ' '
               << entry.relative_scale[0] << ' ' << entry.relative_scale[1] << ' ' << entry.relative_scale[2] << ' '
               << std::quoted(std::string(entry.model_name)) << ' '
               << (entry.enabled ? 1 : 0) << ' ' << (entry.deleted ? 1 : 0) << ' '
               << entry.properties[0] << ' ' << entry.properties[1] << ' '
               << entry.properties[2] << ' ' << entry.properties[3] << ' '
               << std::quoted(has_material ? std::string(material->material_name) : std::string{}) << ' '
               << std::quoted(has_material ? std::string(material->albedo_asset) : std::string{}) << ' '
               << std::quoted(has_material ? std::string(material->normal_asset) : std::string{}) << ' '
               << std::quoted(has_material ? std::string(material->orm_asset) : std::string{}) << ' '
               << (has_material ? material->uv_scale : 1.0f) << ' '
               << (has_material ? material->metallic : 1.0f) << ' '
               << (has_material ? material->roughness : 1.0f) << ' '
               << (has_material ? material->normal_scale : 1.0f) << ' '
               << -1 << ' ' << std::quoted(std::string(entry.editor_group)) << '\n';
    }
    return static_cast<bool>(output);
}

void WorldSceneBuilder::build(ResourceSpine& resources, scene::SceneSpine& scene) {
    const MeshHandle cube = resources.create_mesh(gl::make_cube_mesh());
    const MeshHandle plane = resources.create_mesh(gl::make_plane_mesh(1.0f));
    const MeshHandle sphere = resources.load_obj_mesh("default_pack/models/primitives/sphere_uv.obj");
    const MeshHandle torus = resources.load_obj_mesh("default_pack/models/primitives/torus.obj");
    const MeshHandle ico = resources.load_obj_mesh("default_pack/models/primitives/sphere_ico.obj");
    const MeshHandle cylinder = resources.load_obj_mesh("default_pack/models/primitives/cylinder.obj");
    const MeshHandle capsule = resources.load_obj_mesh("default_pack/models/primitives/capsule.obj");
    const MeshHandle humanoid = resources.load_obj_mesh("default_pack/models/characters/humanoid_static.obj");
    const MeshHandle oak = resources.load_obj_mesh("default_pack/models/nature/tree_oak.obj");
    const MeshHandle pine = resources.load_obj_mesh("default_pack/models/nature/tree_pine.obj");
    const MeshHandle rocks = resources.load_obj_mesh("default_pack/models/nature/rock_cluster.obj");
    const MeshHandle crate = resources.load_obj_mesh("default_pack/models/props/crate.obj");
    const MeshHandle hard_edges = resources.load_obj_mesh("default_pack/models/diagnostics/hard_edges.obj");
    const MeshHandle smooth_edges = resources.load_obj_mesh("default_pack/models/diagnostics/smooth_edges.obj");
    const MeshHandle atlas_cube = resources.load_obj_mesh("default_pack/models/diagnostics/textured_atlas_cube.obj");

    scene.editor_models = {
        {"Cube", cube},
        {"Plane", plane},
        {"UV sphere", sphere},
        {"Icosphere", ico},
        {"Cylinder", cylinder},
        {"Capsule", capsule},
        {"Torus", torus},
        {"Humanoid", humanoid},
        {"Oak tree", oak},
        {"Pine tree", pine},
        {"Rock cluster", rocks},
        {"Crate", crate},
        {"Hard-edge diagnostic", hard_edges},
        {"Smooth diagnostic", smooth_edges},
        {"Atlas cube", atlas_cube}
    };

    const auto white = resources.load_texture("default_pack/textures/defaults/white.png", gl::ColorSpace::srgb);
    const auto flat_normal = resources.load_texture("default_pack/textures/defaults/flat_normal.png", gl::ColorSpace::linear);
    const auto default_orm = resources.load_texture("default_pack/textures/defaults/default_orm.png", gl::ColorSpace::linear);
    const auto default_height = resources.load_texture("default_pack/textures/defaults/default_height.png", gl::ColorSpace::linear);
    const auto default_opacity = resources.load_texture("default_pack/textures/defaults/default_opacity.png", gl::ColorSpace::linear);
    const auto default_emissive = resources.load_texture("default_pack/textures/defaults/default_emissive.png", gl::ColorSpace::linear);
    const MaterialMaps defaults{white, flat_normal, default_orm, default_height, default_opacity};

    const auto concrete_maps = load_pack_maps(resources, "concrete", default_opacity);
    const auto brick_maps = load_pack_maps(resources, "brick", default_opacity);
    const auto wood_maps = load_pack_maps(resources, "wood", default_opacity);
    const auto painted_maps = load_pack_maps(resources, "painted_metal", default_opacity);
    const auto rust_maps = load_pack_maps(resources, "rusted_metal", default_opacity);
    const auto ceramic_maps = load_pack_maps(resources, "ceramic_tile", default_opacity);
    const auto rock_maps = load_pack_maps(resources, "rock", default_opacity);
    const auto foliage_maps = load_pack_maps(resources, "foliage", default_opacity);
    const auto gold_maps = load_curated_maps(resources, "polished_gold", default_opacity);
    const auto glass_maps = load_curated_maps(resources, "frosted_glass", default_opacity);
    const auto fabric_maps = load_curated_maps(resources, "woven_blue_fabric", default_opacity);
    const auto carbon_maps = load_curated_maps(resources, "carbon_fiber", default_opacity);
    const auto aluminum_maps = load_curated_maps(resources, "brushed_aluminum", default_opacity);
    const auto car_paint_maps = load_curated_maps(resources, "car_paint_red", default_opacity);
    const auto rubber_maps = load_curated_maps(resources, "black_rubber", default_opacity);

    constexpr MaterialFeature normal_environment = MaterialFeature::normal_mapping | MaterialFeature::environment;
    constexpr MaterialFeature normal_parallax_environment = normal_environment | MaterialFeature::parallax_mapping;

    const MaterialHandle neutral = add_material(scene, "Neutral architecture", defaults, default_emissive,
        {0.44f, 0.48f, 0.54f}, 0.0f, 0.82f, 0.0f, 0.0f, 1.0f, MaterialFeature::environment);
    const MaterialHandle concrete = add_material(scene, "Concrete", concrete_maps, default_emissive,
        {0.48f, 0.50f, 0.52f}, 0.0f, 0.95f, 0.38f, 0.0f, 6.0f, normal_environment);
    const MaterialHandle stone_path = add_material(scene, "Stone path", concrete_maps, default_emissive,
        {0.42f, 0.43f, 0.44f}, 0.0f, 0.94f, 0.42f, 0.008f, 18.0f, normal_parallax_environment);
    const MaterialHandle roofing = add_material(scene, "Weathered slate roofing", rock_maps, default_emissive,
        {0.34f, 0.37f, 0.40f}, 0.0f, 0.94f, 0.30f, 0.0f, 9.5f, normal_environment);
    const MaterialHandle brick = add_material(scene, "Building-scale parallax brick", brick_maps, default_emissive,
        {0.50f, 0.39f, 0.31f}, 0.0f, 0.88f, 0.46f, 0.016f, 8.0f,
        normal_parallax_environment | MaterialFeature::projected_texture);
    const MaterialHandle brick_detail = add_material(scene, "Detail-scale parallax brick", brick_maps, default_emissive,
        {0.50f, 0.39f, 0.31f}, 0.0f, 0.88f, 0.46f, 0.016f, 3.2f,
        normal_parallax_environment | MaterialFeature::projected_texture);
    const MaterialHandle wood = add_material(scene, "Wood", wood_maps, default_emissive,
        {0.46f, 0.36f, 0.27f}, 0.0f, 0.88f, 0.42f, 0.0f, 1.35f, normal_environment);
    const MaterialHandle bench_wood = add_material(scene, "Bench wood planks", wood_maps, default_emissive,
        {0.42f, 0.31f, 0.22f}, 0.0f, 0.88f, 0.18f, 0.0f, 0.55f, normal_environment);
    const MaterialHandle waterbed = add_material(scene, "Submerged stone", rock_maps, default_emissive,
        {0.16f, 0.20f, 0.21f}, 0.0f, 1.0f, 0.18f, 0.0f, 2.0f, normal_environment);
    const MaterialHandle painted = add_material(scene, "Painted metal", painted_maps, default_emissive,
        {0.34f, 0.42f, 0.54f}, 1.0f, 0.66f, 0.34f, 0.0f, 1.0f,
        normal_environment | MaterialFeature::clearcoat, {}, 0.62f, 0.18f);
    const MaterialHandle rust = add_material(scene, "Rusted metal", rust_maps, default_emissive,
        {0.62f, 0.56f, 0.48f}, 1.0f, 0.95f, 0.65f, 0.0f, 1.2f, normal_environment);
    const MaterialHandle ceramic = add_material(scene, "Ceramic", ceramic_maps, default_emissive,
        {0.72f, 0.78f, 0.84f}, 0.0f, 0.38f, 0.28f, 0.0f, 1.4f, normal_environment);
    const MaterialHandle rock = add_material(scene, "Rock", rock_maps, default_emissive,
        {0.58f, 0.59f, 0.57f}, 0.0f, 0.98f, 0.52f, 0.0f, 1.0f, normal_environment);
    const MaterialHandle foliage = add_material(scene, "Foliage", foliage_maps, default_emissive,
        {0.25f, 0.42f, 0.22f}, 0.0f, 0.95f, 0.38f, 0.0f, 1.0f,
        normal_environment | MaterialFeature::alpha_cutout, {}, 0.0f, 0.12f, 0.0f, 1.5f, 0.32f);
    // The packaged tree OBJs carry authored trunk/leaf colors directly on their
    // vertices and intentionally have no UVs. Keep them opaque and multiply the
    // material by vertex color so the visible pass matches the geometry that
    // already casts directional and point-light shadows.
    const MaterialHandle vertex_colored_trees = add_material(scene, "Vertex-colored trees", defaults, default_emissive,
        {1.0f, 1.0f, 1.0f}, 0.0f, 0.92f, 1.0f, 0.0f, 1.0f,
        normal_environment);
    const MaterialHandle gold = add_material(scene, "Bronze clearcoat", gold_maps, default_emissive,
        {0.82f, 0.58f, 0.22f}, 1.0f, 0.28f, 0.28f, 0.0f, 1.0f,
        normal_environment | MaterialFeature::clearcoat, {}, 0.78f, 0.09f);
    const MaterialHandle glass = add_material(scene, "Architectural glass", glass_maps, default_emissive,
        {0.42f, 0.62f, 0.78f}, 0.0f, 0.16f, 0.22f, 0.0f, 1.0f,
        normal_environment | MaterialFeature::transmission, {}, 0.0f, 0.12f, 0.78f, 1.46f);
    const MaterialHandle carbon = add_material(scene, "Carbon detail", carbon_maps, default_emissive,
        {0.38f, 0.42f, 0.50f}, 1.0f, 0.44f, 0.48f, 0.0f, 2.0f,
        normal_environment | MaterialFeature::clearcoat, {}, 0.35f, 0.20f);
    const MaterialHandle aluminum = add_material(scene, "Brushed aluminum", aluminum_maps, default_emissive,
        {0.78f, 0.80f, 0.82f}, 1.0f, 0.24f, 0.42f, 0.0f, 1.8f,
        normal_environment | MaterialFeature::clearcoat, {}, 0.34f, 0.10f);
    const MaterialHandle car_paint = add_material(scene, "Deep red automotive clearcoat", car_paint_maps, default_emissive,
        {0.68f, 0.11f, 0.08f}, 0.72f, 0.20f, 0.38f, 0.0f, 1.0f,
        normal_environment | MaterialFeature::clearcoat, {}, 0.92f, 0.06f);
    const MaterialHandle rubber = add_material(scene, "Matte black rubber", rubber_maps, default_emissive,
        {0.08f, 0.085f, 0.09f}, 0.0f, 0.92f, 0.30f, 0.0f, 1.0f,
        normal_environment);
    const MaterialHandle emissive_warm = add_material(scene, "Warm emissive", defaults, white,
        {0.18f, 0.07f, 0.02f}, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
        MaterialFeature::none, {2.8f, 1.05f, 0.20f}, 0.0f, 0.12f, 0.0f, 1.5f, 0.0f, true);
    const MaterialHandle emissive_blue = add_material(scene, "Blue spotlight emissive", defaults, white,
        {0.02f, 0.06f, 0.22f}, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
        MaterialFeature::none, {0.25f, 0.65f, 4.8f}, 0.0f, 0.12f, 0.0f, 1.5f, 0.0f, true);
    const MaterialHandle emissive_red = add_material(scene, "Red spotlight emissive", defaults, white,
        {0.24f, 0.015f, 0.01f}, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
        MaterialFeature::none, {5.2f, 0.18f, 0.08f}, 0.0f, 0.12f, 0.0f, 1.5f, 0.0f, true);

    const auto american_flag = resources.load_texture(
        "curated/cloth/american_flag.png", gl::ColorSpace::srgb);
    const auto israeli_flag = resources.load_texture(
        "curated/cloth/israeli_flag.png", gl::ColorSpace::srgb);

    scene.environment = resources.load_cubemap({
        "environment/outdoor_posx.png", "environment/outdoor_negx.png",
        "environment/outdoor_posy.png", "environment/outdoor_negy.png",
        "environment/outdoor_posz.png", "environment/outdoor_negz.png"}, gl::ColorSpace::srgb);

    scene.sun.direction = math::normalize(math::Vec3{-0.42f, -1.0f, -0.28f});
    scene.sun.color = {0.96f, 0.92f, 0.84f};

    // Four local lights are placed inside visible scene zones. They illuminate nearby
    // occluders and materials instead of sitting outside the useful camera volume.
    scene.point_light_count = 4;
    scene.point_lights[0] = {{-11.2f, 4.0f, -12.2f}, 7.0f, {0.96f, 0.72f, 0.48f}, 8.0f};
    scene.point_lights[1] = {{-3.7f, 4.1f, -12.0f}, 7.0f, {0.48f, 0.68f, 1.0f}, 7.2f};
    scene.point_lights[2] = {{10.8f, 4.0f, -12.1f}, 7.5f, {0.92f, 0.48f, 0.34f}, 7.5f};
    scene.point_lights[3] = {{-15.0f, 1.25f, 8.0f}, 5.5f, {1.0f, 0.30f, 0.07f}, 6.0f};

    const math::Vec3 camera_room_target{3.75f, 1.85f, -16.80f};
    scene.spotlight.position = {1.05f, 4.55f, -13.10f};
    scene.spotlight.authored_direction = math::normalize(camera_room_target - scene.spotlight.position);
    scene.spotlight.direction = scene.spotlight.authored_direction;
    scene.spotlight.color = {1.0f, 0.035f, 0.02f};
    scene.spotlight.intensity = 16.0f;
    scene.spotlight.range = 15.0f;
    scene.spotlight.inner_cosine = 0.95f;
    scene.spotlight.outer_cosine = 0.80f;
    scene.spotlight.rotation_speed = 0.58f;
    scene.spotlight.dual_sided = true;

    scene.secondary_spotlight.position = {6.45f, 4.55f, -13.10f};
    scene.secondary_spotlight.authored_direction = math::normalize(camera_room_target - scene.secondary_spotlight.position);
    scene.secondary_spotlight.direction = scene.secondary_spotlight.authored_direction;
    scene.secondary_spotlight.color = {0.025f, 0.12f, 1.0f};
    scene.secondary_spotlight.intensity = 16.0f;
    scene.secondary_spotlight.range = 15.0f;
    scene.secondary_spotlight.inner_cosine = 0.95f;
    scene.secondary_spotlight.outer_cosine = 0.80f;
    scene.secondary_spotlight.rotation_speed = -0.52f;
    scene.secondary_spotlight.dual_sided = true;

    // Dedicated one-sided projector for the brick/parallax room. The red and blue
    // camera-room spotlights remain independent and never own the projected cookie.
    scene.projector_spotlight.position = {-3.75f, 5.55f, -10.0f};
    scene.projector_spotlight.authored_direction = math::normalize(math::Vec3{0.0f, -0.18f, -1.0f});
    scene.projector_spotlight.direction = scene.projector_spotlight.authored_direction;
    scene.projector_spotlight.color = {1.0f, 0.82f, 0.56f};
    scene.projector_spotlight.intensity = 10.0f;
    scene.projector_spotlight.range = 14.0f;
    scene.projector_spotlight.inner_cosine = 0.94f;
    scene.projector_spotlight.outer_cosine = 0.80f;
    scene.projector_spotlight.rotation_speed = 0.0f;
    scene.projector_spotlight.dual_sided = false;

    // -------------------------------------------------------------------------
    // Tier-0 display building
    // -------------------------------------------------------------------------
    // The open-front building is one integrated scene, but its four rooms give each
    // feature family enough physical space to remain readable from the courtyard.
    scene.objects.push_back(object("Showcase foundation", cube, concrete,
        {0.0f, 0.14f, -14.0f}, {30.0f, 0.28f, 11.0f}));
    scene.objects.push_back(object("Showcase floor", cube, stone_path,
        {0.0f, 0.34f, -14.0f}, {29.3f, 0.12f, 10.3f}));
    scene.objects.push_back(object("Showcase expanded rear wall", cube, concrete,
        {0.0f, 3.35f, -19.46f}, {30.4f, 6.70f, 0.34f}));
    scene.objects.push_back(object("Showcase rear wall cap", cube, aluminum,
        {0.0f, 6.76f, -19.40f}, {30.7f, 0.12f, 0.22f}));
    scene.objects.push_back(object("Showcase west wall", cube, concrete,
        {-15.0f, 3.25f, -14.0f}, {0.30f, 6.5f, 11.0f}));
    scene.objects.push_back(object("Showcase east wall", cube, concrete,
        {15.0f, 3.25f, -14.0f}, {0.30f, 6.5f, 11.0f}));
    for (const float x : {-7.5f, 0.0f, 7.5f}) {
        scene.objects.push_back(object("Showcase room divider", cube, aluminum,
            {x, 2.95f, -14.0f}, {0.18f, 5.8f, 10.4f}));
    }
    scene.objects.push_back(object("Showcase front header", cube, aluminum,
        {0.0f, 6.18f, -8.58f}, {30.2f, 0.28f, 0.30f}));
    scene.objects.push_back(object("Showcase rear roof", cube, roofing,
        {0.0f, 6.42f, -16.6f}, {30.4f, 0.25f, 5.5f}));
    for (const float x : {-11.25f, -3.75f, 3.75f, 11.25f}) {
        auto canopy = object("Showcase glass canopy", cube, glass,
            {x, 6.36f, -11.35f}, {7.10f, 0.10f, 5.0f});
        canopy.casts_shadow = false;
        canopy.double_sided = true;
        scene.objects.push_back(std::move(canopy));
    }

    const auto add_pedestal = [&](math::Vec3 position, math::Vec3 size) {
        scene.objects.push_back(object("Display pedestal", cube, concrete, position, size));
        scene.objects.push_back(object("Brushed aluminum pedestal trim", cube, aluminum,
            position + math::Vec3{0.0f, size.y * 0.53f, 0.0f},
            {size.x * 1.04f, 0.06f, size.z * 1.04f}));
    };

    const auto add_screen_frame = [&](math::Vec3 bottom_center, float width, float height) {
        const float center_y = bottom_center.y + height * 0.5f;
        const float z = bottom_center.z - 0.035f;
        scene.objects.push_back(object("RTT screen top frame", cube, aluminum,
            {bottom_center.x, bottom_center.y + height + 0.08f, z}, {width + 0.34f, 0.16f, 0.18f}));
        scene.objects.push_back(object("RTT screen bottom frame", cube, aluminum,
            {bottom_center.x, bottom_center.y - 0.08f, z}, {width + 0.34f, 0.16f, 0.18f}));
        scene.objects.push_back(object("RTT screen left frame", cube, aluminum,
            {bottom_center.x - width * 0.5f - 0.08f, center_y, z}, {0.16f, height + 0.16f, 0.18f}));
        scene.objects.push_back(object("RTT screen right frame", cube, aluminum,
            {bottom_center.x + width * 0.5f + 0.08f, center_y, z}, {0.16f, height + 0.16f, 0.18f}));
    };

    const auto add_light_fixture = [&](math::Vec3 position, MaterialHandle emissive) {
        scene.objects.push_back(object("Correctly oriented light base", cylinder, painted,
            position + math::Vec3{0.0f, 0.22f, 0.0f}, {0.42f, 0.42f, 0.42f}));
        scene.objects.push_back(object("Correctly oriented light pole", cube, painted,
            position + math::Vec3{0.0f, 2.30f, 0.0f}, {0.16f, 4.35f, 0.16f}));
        scene.objects.push_back(object("Correctly oriented light crossarm", cube, painted,
            position + math::Vec3{0.0f, 4.42f, 0.0f}, {1.55f, 0.14f, 0.14f}));
        for (const float x : {-0.62f, 0.62f}) {
            scene.objects.push_back(object("Light pendant", cube, painted,
                position + math::Vec3{x, 4.12f, 0.0f}, {0.08f, 0.48f, 0.08f}));
            auto globe = object("Transmission lamp globe", sphere, glass,
                position + math::Vec3{x, 3.78f, 0.0f}, {0.34f, 0.34f, 0.34f});
            globe.casts_shadow = false;
            scene.objects.push_back(std::move(globe));
            auto bulb = object("Emissive lamp bulb", ico, emissive,
                position + math::Vec3{x, 3.78f, 0.0f}, {0.12f, 0.12f, 0.12f});
            bulb.casts_shadow = false;
            scene.objects.push_back(std::move(bulb));
        }
    };

    const auto add_room_pendant = [&](std::size_t light_index, MaterialHandle emissive) {
        const auto& light = scene.point_lights[light_index];
        const math::Vec3 light_position = light.position;
        scene.objects.push_back(object("Room light ceiling mount", cylinder, aluminum,
            light_position + math::Vec3{0.0f, 1.82f, 0.0f}, {0.32f, 0.18f, 0.32f}));
        scene.objects.push_back(object("Room light cable", cube, rubber,
            light_position + math::Vec3{0.0f, 0.88f, 0.0f}, {0.035f, 1.70f, 0.035f}));
        auto globe = object("Room point-light glass globe", sphere, glass,
            light_position, {0.34f, 0.34f, 0.34f});
        globe.casts_shadow = false;
        scene.objects.push_back(std::move(globe));
        auto bulb = object("Room point-light emissive bulb", ico, emissive,
            light_position, {0.13f, 0.13f, 0.13f});
        bulb.editor_group = "Point light " + std::to_string(light_index + 1u);
        bulb.editor_binding = scene::EditorBindingKind::point_light;
        bulb.editor_binding_index = light_index;
        bulb.editor_properties = {light.radius, light.intensity, 0.0f, 0.0f};
        bulb.casts_shadow = false;
        scene.objects.push_back(std::move(bulb));
    };
    add_room_pendant(0u, emissive_warm);
    add_room_pendant(1u, emissive_blue);
    add_room_pendant(2u, emissive_warm);

    // Room 1: metallic PBR and close local lighting.
    add_pedestal({-11.25f, 0.78f, -14.0f}, {4.8f, 0.78f, 3.4f});
    auto aluminum_torus = object("Brushed aluminum torus", torus, aluminum,
        {-11.25f, 2.05f, -14.0f}, {2.25f, 2.25f, 2.25f});
    aluminum_torus.angular_velocity = {0.0f, 0.34f, 0.0f};
    scene.objects.push_back(std::move(aluminum_torus));
    auto clearcoat_sphere = object("Automotive clearcoat sphere", sphere, car_paint,
        {-13.2f, 1.75f, -12.25f}, {1.18f, 1.18f, 1.18f});
    clearcoat_sphere.angular_velocity = {0.0f, 0.22f, 0.0f};
    scene.objects.push_back(std::move(clearcoat_sphere));
    scene.objects.push_back(object("Rubber roughness reference", cylinder, rubber,
        {-9.25f, 1.45f, -12.35f}, {1.15f, 1.65f, 1.15f}));
    for (const float x : {-12.75f, -11.25f, -9.75f}) {
        scene.objects.push_back(object("Metal shadow occluder fin", cube, aluminum,
            {x, 2.10f, -16.75f}, {0.24f, 3.25f, 1.05f},
            {0.0f, math::radians(18.0f), 0.0f}));
    }

    // Room 2: planar parallax receiver, projected texture, and visible shadow depth.
    add_pedestal({-3.75f, 0.72f, -14.0f}, {5.0f, 0.66f, 3.5f});
    add_wall_with_parallax_face(scene, cube, plane, concrete, brick_detail,
        {-3.75f, 2.55f, -18.96f}, 6.6f, 4.5f);
    auto parallax_panel = object("Parallax depth comparison panel", plane, brick_detail,
        {-3.75f, 1.05f, -13.60f}, {1.95f, 1.0f, 1.55f},
        {math::radians(90.0f), 0.0f, 0.0f});
    parallax_panel.casts_shadow = false;
    scene.objects.push_back(std::move(parallax_panel));
    scene.objects.push_back(object("Projected light occluder A", cube, concrete,
        {-5.35f, 2.15f, -12.0f}, {0.45f, 3.5f, 0.45f}));
    scene.objects.push_back(object("Projected light occluder B", cube, concrete,
        {-2.15f, 1.65f, -12.5f}, {0.65f, 2.5f, 0.65f}));

    // Room 3: useful render-to-texture feeds, not decorative black panels.
    // Two wall monitors show independent cameras; a separate courtyard mirror uses
    // a reflected main-camera view so Codex can ingest all three FBO ownership paths.
    const math::Vec3 security_screen{2.15f, 1.28f, -18.88f};
    const math::Vec3 overhead_screen{5.30f, 1.28f, -18.88f};
    const math::Vec3 upper_camera_screen{3.725f, 3.42f, -18.88f};
    add_screen_frame(security_screen, 2.75f, 1.60f);
    add_screen_frame(overhead_screen, 2.75f, 1.60f);
    add_screen_frame(upper_camera_screen, 4.15f, 1.25f);
    scene.rtt_displays.push_back({
        .name = "Live security-camera render target",
        .editor_group = "Security RTT display",
        .transform = {security_screen, {}, {2.75f, 1.60f, 1.0f}},
        .feed = scene::RttFeed::security_camera,
        .preset_mask = world_mask
    });
    scene.rtt_displays.push_back({
        .name = "Overhead site-camera render target",
        .editor_group = "Overhead RTT display",
        .transform = {overhead_screen, {}, {2.75f, 1.60f, 1.0f}},
        .feed = scene::RttFeed::overhead_camera,
        .preset_mask = world_mask
    });
    scene.rtt_displays.push_back({
        .name = "Ceiling security-camera master display",
        .editor_group = "Ceiling RTT display",
        .transform = {upper_camera_screen, {}, {4.15f, 1.25f, 1.0f}},
        .feed = scene::RttFeed::ceiling_camera,
        .preset_mask = world_mask
    });
    scene.security_camera = {
        .position = {-15.0f, 6.6f, 12.5f},
        .target = {15.5f, 0.8f, 5.0f},
        .up = {0.0f, 1.0f, 0.0f},
        .vertical_fov_degrees = 58.0f
    };
    scene.overhead_camera = {
        .position = {0.0f, 30.0f, 2.0f},
        .target = {0.0f, 0.0f, -4.0f},
        .up = {0.0f, 0.0f, -1.0f},
        .vertical_fov_degrees = 50.0f
    };
    scene.ceiling_camera = {
        .position = {1.22f, 5.20f, -9.60f},
        .target = {3.75f, 1.40f, -14.25f},
        .up = {0.0f, 1.0f, 0.0f},
        .vertical_fov_degrees = 58.0f
    };
    const auto camera_euler = [](math::Vec3 position, math::Vec3 target) noexcept {
        const math::Vec3 direction = math::normalize(target - position);
        return math::Vec3{
            -std::asin(std::clamp(direction.y, -1.0f, 1.0f)),
            std::atan2(direction.x, direction.z),
            0.0f
        };
    };

    // Ceiling feed: compact PTZ/dome camera aimed at the RTT room.
    const math::Vec3 ceiling_direction = math::normalize(scene.ceiling_camera.target - scene.ceiling_camera.position);
    const math::Vec3 ceiling_rotation = camera_euler(scene.ceiling_camera.position, scene.ceiling_camera.target);
    scene.objects.push_back(object("Ceiling PTZ mount", cylinder, aluminum,
        scene.ceiling_camera.position + math::Vec3{0.0f, 0.30f, 0.0f},
        {0.34f, 0.16f, 0.34f}));
    scene.objects.push_back(object("Ceiling PTZ dome", sphere, carbon,
        scene.ceiling_camera.position - ceiling_direction * 0.20f + math::Vec3{0.0f, 0.08f, 0.0f},
        {0.38f, 0.30f, 0.38f}, ceiling_rotation));
    scene.objects.push_back(object("Ceiling PTZ lens", cylinder, glass,
        scene.ceiling_camera.position - ceiling_direction * 0.10f,
        {0.14f, 0.24f, 0.14f},
        {ceiling_rotation.x + math::radians(90.0f), ceiling_rotation.y, 0.0f}));

    // Original lower security feed: a visibly different long-body outdoor camera.
    const math::Vec3 security_direction = math::normalize(scene.security_camera.target - scene.security_camera.position);
    const math::Vec3 security_rotation = camera_euler(scene.security_camera.position, scene.security_camera.target);
    scene.objects.push_back(object("Outdoor security-camera wall bracket", cube, aluminum,
        scene.security_camera.position - security_direction * 0.78f,
        {0.12f, 0.12f, 0.72f}, security_rotation));
    scene.objects.push_back(object("Outdoor security-camera body", cube, carbon,
        scene.security_camera.position - security_direction * 0.36f,
        {0.42f, 0.28f, 0.62f}, security_rotation));
    scene.objects.push_back(object("Outdoor security-camera sun hood", cube, aluminum,
        scene.security_camera.position - security_direction * 0.20f + math::Vec3{0.0f, 0.18f, 0.0f},
        {0.48f, 0.08f, 0.72f}, security_rotation));
    scene.objects.push_back(object("Outdoor security-camera lens", cylinder, glass,
        scene.security_camera.position - security_direction * 0.10f,
        {0.17f, 0.25f, 0.17f},
        {security_rotation.x + math::radians(90.0f), security_rotation.y, 0.0f}));

    // Camera-attached player representation. It stays below and slightly behind the
    // active camera, so mirrors and RTT cameras see a coherent humanoid while the
    // first-person view does not stare through a mannequin in the display room.
    auto player = object("Camera-attached player humanoid", humanoid, painted,
        {}, {0.72f, 0.72f, 0.72f});
    player.camera_attached = true;
    player.camera_local_offset = {0.0f, -1.82f, -0.26f};
    player.camera_rotation_offset = {0.0f, 0.0f, 0.0f};
    player.casts_shadow = true;
    scene.objects.push_back(std::move(player));

    // Room 4: transmission, clearcoat and environment response.
    add_pedestal({11.25f, 0.76f, -14.0f}, {5.0f, 0.72f, 3.7f});
    auto transmission_lens = object("Transmission crystal lens", ico, glass,
        {9.65f, 1.95f, -14.0f}, {1.12f, 1.45f, 1.12f},
        {math::radians(12.0f), math::radians(24.0f), math::radians(-8.0f)});
    transmission_lens.casts_shadow = false;
    transmission_lens.double_sided = true;
    transmission_lens.angular_velocity = {0.0f, 0.16f, 0.0f};
    scene.objects.push_back(std::move(transmission_lens));
    auto paint_form = object("Clearcoat automotive form", capsule, car_paint,
        {11.25f, 1.65f, -14.0f}, {2.15f, 1.15f, 1.15f},
        {0.0f, math::radians(90.0f), 0.0f});
    paint_form.angular_velocity = {0.0f, 0.18f, 0.0f};
    scene.objects.push_back(std::move(paint_form));
    scene.objects.push_back(object("Carbon fiber inner plinth", cube, carbon,
        {11.25f, 0.95f, -14.0f}, {3.0f, 0.42f, 1.65f}));

    // Correctly oriented exterior fixtures frame the building entrance.
    add_light_fixture({-13.8f, 0.0f, -7.0f}, emissive_warm);
    add_light_fixture({13.8f, 0.0f, -7.0f}, emissive_blue);

    // -------------------------------------------------------------------------
    // Courtyard, camp, flag and Tier-0 prop/foliage systems
    // -------------------------------------------------------------------------
    scene.objects.push_back(object("Unified courtyard platform", cube, stone_path,
        {0.0f, -0.20f, 1.0f}, {17.90f, 0.36f, 8.90f}));
    scene.objects.push_back(object("North retaining face", cube, concrete,
        {0.0f, -0.38f, -8.35f}, {17.90f, 0.72f, 0.44f}));
    scene.objects.push_back(object("South retaining face", cube, concrete,
        {0.0f, -0.38f, 10.35f}, {17.90f, 0.72f, 0.44f}));
    scene.objects.push_back(object("West retaining face", cube, concrete,
        {-18.35f, -0.38f, 1.0f}, {0.44f, 0.72f, 8.90f}));
    scene.objects.push_back(object("East retaining face", cube, concrete,
        {18.35f, -0.38f, 1.0f}, {0.44f, 0.72f, 8.90f}));

    // Freestanding mirror RTT. It faces the open courtyard, so the reflected camera
    // never sits behind a solid wall and the feed remains useful without clip-plane
    // extensions or hidden scene hacks.
    const math::Vec3 mirror_screen{5.5f, 0.70f, 8.4f};
    add_screen_frame(mirror_screen, 4.4f, 2.7f);
    scene.rtt_displays.push_back({
        .name = "Freestanding planar mirror render target",
        .editor_group = "Planar mirror display",
        .transform = {mirror_screen, {0.0f, math::radians(180.0f), 0.0f}, {4.4f, 2.7f, 1.0f}},
        .feed = scene::RttFeed::planar_mirror,
        .preset_mask = world_mask
    });
    scene.mirror_plane = {
        .center = {mirror_screen.x, mirror_screen.y + 1.35f, mirror_screen.z},
        .normal = {0.0f, 0.0f, -1.0f},
        .up = {0.0f, 1.0f, 0.0f}
    };
    scene.objects.push_back(object("Opaque planar mirror backing", cube, rubber,
        {mirror_screen.x, mirror_screen.y + 1.35f, mirror_screen.z + 0.115f},
        {4.56f, 2.86f, 0.16f}));
    scene.objects.push_back(object("Mirror support left", cube, aluminum,
        {mirror_screen.x - 2.05f, 0.35f, mirror_screen.z + 0.12f}, {0.16f, 1.35f, 0.22f}));
    scene.objects.push_back(object("Mirror support right", cube, aluminum,
        {mirror_screen.x + 2.05f, 0.35f, mirror_screen.z + 0.12f}, {0.16f, 1.35f, 0.22f}));

    constexpr math::Vec3 fire_center{-15.0f, 0.20f, 8.0f};
    for (int index = 0; index < 9; ++index) {
        const float angle = static_cast<float>(index) * 0.69813170f;
        const float radius = (index % 2 == 0) ? 0.82f : 0.70f;
        const float y = (index % 3 == 0) ? 0.06f : 0.02f;
        const math::Vec3 pos = fire_center + math::Vec3{std::cos(angle) * radius, y, std::sin(angle) * radius};
        const math::Vec3 scale = (index % 2 == 0)
            ? math::Vec3{0.24f, 0.18f, 0.20f}
            : math::Vec3{0.19f, 0.14f, 0.17f};
        scene.objects.push_back(object("Campfire stone ring", rocks, rock,
            pos, scale,
            {math::radians(8.0f * static_cast<float>(index % 3)),
             angle + math::radians(12.0f * static_cast<float>(index)),
             math::radians(-6.0f + 3.0f * static_cast<float>(index % 4))}));
    }
    scene.objects.push_back(object("Campfire crossed log A", cube, wood,
        fire_center + math::Vec3{0.0f, 0.15f, 0.0f}, {1.05f, 0.14f, 0.16f},
        {0.0f, math::radians(36.0f), 0.0f}));
    scene.objects.push_back(object("Campfire crossed log B", cube, wood,
        fire_center + math::Vec3{0.0f, 0.18f, 0.0f}, {1.05f, 0.14f, 0.16f},
        {0.0f, math::radians(-36.0f), 0.0f}));
    auto fire_core = object("Campfire emissive core", ico, emissive_warm,
        fire_center + math::Vec3{0.0f, 0.40f, 0.0f}, {0.30f, 0.42f, 0.30f});
    fire_core.casts_shadow = false;
    scene.objects.push_back(std::move(fire_core));

    // The manually corrected east bench is the canonical source. Its tighter end
    // frames and removed braces are rotated unchanged around the campfire.
    const auto add_bench = [&](math::Vec3 center, float yaw,
                               bool explodable, std::string_view editor_group) {
        const auto push_part = [&](scene::RenderObject part, math::Vec3 explode_local,
                                   bool visible = true) {
            if (explodable)
                part.explode_direction = rotate_y_offset(explode_local, yaw);
            part.editor_group = std::string(editor_group);
            part.visible = visible;
            part.editor_deleted = !visible;
            scene.objects.push_back(std::move(part));
        };

        push_part(object("Campfire bench connected seat", cube, bench_wood,
            center + rotate_y_offset({0.0f, 0.58f, 0.0f}, yaw),
            {1.84f, 0.12f, 0.50f}, {0.0f, yaw, 0.0f}),
            {0.0f, 0.16f, 0.0f});
        push_part(object("Campfire bench connected backrest", cube, bench_wood,
            center + rotate_y_offset({0.0f, 1.10f, 0.2352053f}, yaw),
            {1.78f, 0.34f, 0.09f}, {0.0f, yaw, 0.0f}),
            {0.0f, 0.54f, 0.2352053f});

        constexpr std::array<float, 2> leg_offsets{-0.4738693f, 0.5221634f};
        constexpr std::array<float, 2> brace_offsets{-0.86f, 0.86f};
        for (std::size_t frame = 0; frame < leg_offsets.size(); ++frame) {
            const float offset = leg_offsets[frame];
            const float brace_offset = brace_offsets[frame];
            const float side = offset < 0.0f ? -1.0f : 1.0f;
            push_part(object("Campfire bench leg", cube, bench_wood,
                center + rotate_y_offset({offset, 0.31f, 0.0f}, yaw),
                {0.18f, 0.58f, 0.42f}, {0.0f, yaw, 0.0f}),
                {side * 0.42f, -0.26f, 0.0f});
            push_part(object("Campfire bench back support", cube, bench_wood,
                center + rotate_y_offset({offset, 0.82f, 0.30f}, yaw),
                {0.13f, 0.58f, 0.13f}, {0.0f, yaw, 0.0f}),
                {side * 0.40f, 0.18f, 0.26f});
            // The corrected source bench removed both end braces. Keep hidden editor
            // records so old selections and scene indices remain stable.
            push_part(object("Campfire bench end brace", cube, bench_wood,
                center + rotate_y_offset({brace_offset, 0.53f, 0.15f}, yaw),
                {0.20f, 0.08f, 0.40f}, {0.0f, yaw, 0.0f}),
                {side * 0.42f, 0.0f, 0.14f}, false);
        }

        push_part(object("Campfire bench longitudinal stretcher", cube, bench_wood,
            center + rotate_y_offset({0.0f, 0.24f, 0.0f}, yaw),
            {1.04f, 0.09f, 0.10f}, {0.0f, yaw, 0.0f}),
            {0.0f, -0.36f, 0.0f});
    };
    add_bench(fire_center + math::Vec3{0.0f, 0.0f, 2.35f}, 0.0f, true, "Camp bench south");
    add_bench(fire_center + math::Vec3{0.0f, 0.0f, -2.35f}, math::radians(180.0f), false, "Camp bench north");
    add_bench(fire_center + math::Vec3{2.35f, 0.0f, 0.0f}, math::radians(90.0f), false, "Camp bench east");
    add_bench(fire_center + math::Vec3{-2.35f, 0.0f, 0.0f}, math::radians(-90.0f), false, "Camp bench west");

    // A detailed American flag uses the cloth solver. The pole, halyard, cross-piece
    // and finial are authored individually so every element is correctly oriented.
    constexpr math::Vec3 flag_base{-18.5f, 0.0f, -3.5f};
    scene.objects.push_back(object("Flag pole base", cylinder, concrete,
        flag_base + math::Vec3{0.0f, 0.25f, 0.0f}, {0.55f, 0.42f, 0.55f}));
    scene.objects.push_back(object("Flag pole", cube, aluminum,
        flag_base + math::Vec3{0.0f, 3.25f, 0.0f}, {0.12f, 6.2f, 0.12f}));
    scene.objects.push_back(object("Flag pole halyard", cube, rubber,
        flag_base + math::Vec3{0.10f, 3.35f, 0.0f}, {0.018f, 5.7f, 0.018f}));
    scene.objects.push_back(object("Flag pole finial", sphere, gold,
        flag_base + math::Vec3{0.0f, 6.48f, 0.0f}, {0.16f, 0.16f, 0.16f}));
    scene.cloth_objects.push_back({
        .name = "Detailed American flag cloth",
        .origin = flag_base + math::Vec3{0.07f, 6.18f, 0.0f},
        .right_axis = {1.0f, 0.0f, 0.0f},
        .down_axis = {0.0f, -1.0f, 0.0f},
        .tint = {1.0f, 1.0f, 1.0f},
        .albedo = american_flag,
        .normal = fabric_maps.normal,
        .orm = fabric_maps.orm,
        .uv_scale = {1.0f, 1.0f},
        .width = 3.8f,
        .height = 2.0f,
        .wind_response = 2.40f,
        .initial_billow = 0.16f,
        .gravity_scale = 0.70f,
        .columns = 48,
        .rows = 26,
        .pin_mode = scene::ClothPinMode::left_edge,
        .preset_mask = world_mask
    });

    constexpr math::Vec3 unity_flag_base{18.5f, 0.0f, -3.5f};
    scene.objects.push_back(object("Unity flag pole base", cylinder, concrete,
        unity_flag_base + math::Vec3{0.0f, 0.25f, 0.0f}, {0.55f, 0.42f, 0.55f}));
    scene.objects.push_back(object("Unity flag pole", cube, aluminum,
        unity_flag_base + math::Vec3{0.0f, 3.25f, 0.0f}, {0.12f, 6.2f, 0.12f}));
    scene.objects.push_back(object("Unity flag pole halyard", cube, rubber,
        unity_flag_base + math::Vec3{-0.10f, 3.35f, 0.0f}, {0.018f, 5.7f, 0.018f}));
    scene.objects.push_back(object("Unity flag pole finial", sphere, gold,
        unity_flag_base + math::Vec3{0.0f, 6.48f, 0.0f}, {0.16f, 0.16f, 0.16f}));
    scene.cloth_objects.push_back({
        .name = "Detailed Israeli flag cloth",
        .origin = unity_flag_base + math::Vec3{-0.07f, 6.18f, 0.0f},
        .right_axis = {-1.0f, 0.0f, 0.0f},
        .down_axis = {0.0f, -1.0f, 0.0f},
        .tint = {1.0f, 1.0f, 1.0f},
        .albedo = israeli_flag,
        .normal = fabric_maps.normal,
        .orm = fabric_maps.orm,
        .uv_scale = {1.0f, 1.0f},
        .width = 3.8f,
        .height = 2.0f,
        .wind_response = 2.40f,
        .initial_billow = 0.16f,
        .gravity_scale = 0.70f,
        .columns = 48,
        .rows = 26,
        .pin_mode = scene::ClothPinMode::left_edge,
        .preset_mask = world_mask
    });

    // A second cloth surface is integrated as the building awning rather than a flat
    // billboard card. It uses the same constrained solver with a different pin mode.
    scene.objects.push_back(object("Awning support beam", cube, aluminum,
        {-1.8f, 5.75f, -8.35f}, {4.2f, 0.12f, 0.12f}));
    scene.cloth_objects.push_back({
        .name = "Woven entrance awning cloth",
        .origin = {-3.80f, 5.68f, -8.28f},
        .right_axis = {1.0f, 0.0f, 0.0f},
        .down_axis = {0.0f, -0.96f, 0.28f},
        .tint = {0.28f, 0.42f, 0.72f},
        .albedo = fabric_maps.albedo,
        .normal = fabric_maps.normal,
        .orm = fabric_maps.orm,
        .width = 4.0f,
        .height = 1.35f,
        .wind_response = 1.10f,
        .initial_billow = 0.075f,
        .gravity_scale = 0.72f,
        .columns = 36,
        .rows = 16,
        .pin_mode = scene::ClothPinMode::top_edge,
        .preset_mask = world_mask
    });

    // Loading bay: the twelve crates are one mesh/material draw with either ordinary
    // instancing (Tier 0) or optional indirect submission (desktop toggle).
    scene.objects.push_back(object("Instanced loading bay slab", cube, concrete,
        {18.6f, 0.16f, -10.0f}, {6.4f, 0.28f, 5.2f}));
    // Build the barrel from consistently authored primitives. The cylinder and
    // torus meshes both use local Z as their axis, so every component receives the
    // same X rotation into Epoch's Y-up world. This avoids the source OBJ whose
    // body and hoop submeshes use conflicting local orientations.
    constexpr math::Vec3 barrel_center{21.0f, 1.34f, -8.325f};
    constexpr math::Vec3 barrel_axis_rotation{math::radians(90.0f), 0.0f, 0.0f};
    scene.objects.push_back(object("Wooden barrel lower stave section", cylinder, wood,
        barrel_center + math::Vec3{0.0f, -0.56f, 0.0f}, {1.34f, 1.34f, 0.56f},
        barrel_axis_rotation));
    scene.objects.push_back(object("Wooden barrel bulged center section", cylinder, wood,
        barrel_center, {1.52f, 1.52f, 0.74f}, barrel_axis_rotation));
    scene.objects.push_back(object("Wooden barrel upper stave section", cylinder, wood,
        barrel_center + math::Vec3{0.0f, 0.56f, 0.0f}, {1.34f, 1.34f, 0.56f},
        barrel_axis_rotation));

    for (const float hoop_y : {-0.80f, -0.37f, 0.37f, 0.80f}) {
        scene.objects.push_back(object("Barrel forged-metal hoop", torus, rust,
            barrel_center + math::Vec3{0.0f, hoop_y, 0.0f},
            {1.06f, 1.06f, 0.30f}, barrel_axis_rotation));
    }
    scene.objects.push_back(object("Wooden barrel top cap", cylinder, wood,
        barrel_center + math::Vec3{0.0f, 0.86f, 0.0f}, {1.22f, 1.22f, 0.10f},
        barrel_axis_rotation));
    scene.objects.push_back(object("Wooden barrel bottom cap", cylinder, wood,
        barrel_center + math::Vec3{0.0f, -0.86f, 0.0f}, {1.22f, 1.22f, 0.10f},
        barrel_axis_rotation));

    // -------------------------------------------------------------------------
    // Architectural pool, water border and distant terrain water
    // -------------------------------------------------------------------------
    constexpr math::Vec3 pool_center{16.0f, 0.0f, 5.0f};
    scene.objects.push_back(object("Reflecting pool submerged bed", cube, waterbed,
        pool_center + math::Vec3{0.0f, -0.30f, 0.0f}, {11.0f, 0.34f, 6.5f}));
    scene.objects.push_back(object("Reflecting pool north coping", cube, concrete,
        pool_center + math::Vec3{0.0f, 0.08f, -3.55f}, {11.8f, 0.38f, 0.42f}));
    scene.objects.push_back(object("Reflecting pool south coping", cube, concrete,
        pool_center + math::Vec3{0.0f, 0.08f, 3.55f}, {11.8f, 0.38f, 0.42f}));
    scene.objects.push_back(object("Reflecting pool west coping", cube, concrete,
        pool_center + math::Vec3{-5.70f, 0.08f, 0.0f}, {0.42f, 0.38f, 6.7f}));
    scene.objects.push_back(object("Reflecting pool east coping", cube, concrete,
        pool_center + math::Vec3{5.70f, 0.08f, 0.0f}, {0.42f, 0.38f, 6.7f}));
    scene.objects.push_back(object("Pool fountain plinth", cylinder, concrete,
        pool_center + math::Vec3{0.0f, 0.22f, 0.0f}, {0.85f, 0.50f, 0.85f}));
    scene.objects.push_back(object("Pool brushed metal sculpture", torus, aluminum,
        pool_center + math::Vec3{0.0f, 1.45f, 0.0f}, {1.75f, 1.75f, 1.75f}));

    scene.water_surfaces.push_back({
        .name = "Bordered architectural reflecting pool",
        .editor_group = "Architectural pool",
        .transform = {pool_center + math::Vec3{0.0f, 0.02f, 0.0f}, {}, {5.45f, 1.0f, 3.15f}},
        .shallow_color = {0.085f, 0.28f, 0.32f},
        .deep_color = {0.018f, 0.065f, 0.090f},
        .flow_direction = {0.40f, 1.0f},
        .wave_amplitude = 0.050f,
        .wave_speed = 0.58f,
        .roughness = 0.060f,
        .opacity = 0.91f,
        .foam_strength = 0.34f,
        .kind = scene::WaterSurfaceKind::pool,
        .preset_mask = world_mask
    });
    scene.water_surfaces.push_back({
        .name = "Distant terrain lake",
        .editor_group = "Distant terrain water",
        .transform = {{0.0f, -0.68f, -30.0f}, {}, {24.0f, 1.0f, 7.5f}},
        .shallow_color = {0.045f, 0.20f, 0.26f},
        .deep_color = {0.008f, 0.035f, 0.070f},
        .flow_direction = {0.85f, 0.32f},
        .wave_amplitude = 0.105f,
        .wave_speed = 0.48f,
        .roughness = 0.105f,
        .opacity = 0.89f,
        .foam_strength = 0.42f,
        .kind = scene::WaterSurfaceKind::pond,
        .preset_mask = world_mask
    });
    // Landscape props are kept at the outer terrain ring so they frame rather than
    // crowd the room labels and courtyard navigation.
    for (const auto& [mesh, position, scale, rotation, pick_height] : std::array<std::tuple<MeshHandle, math::Vec3, float, float, float>, 5>{
        std::tuple<MeshHandle, math::Vec3, float, float, float>{pine, {27.0f, -1.0f, 12.0f}, 1.35f, -22.0f, 2.75f},
        std::tuple<MeshHandle, math::Vec3, float, float, float>{oak, {25.0f, -1.0f, -21.0f}, 1.35f, 62.0f, 2.20f},
        std::tuple<MeshHandle, math::Vec3, float, float, float>{pine, {-25.0f, -1.0f, -22.0f}, 1.42f, 105.0f, 2.75f},
        std::tuple<MeshHandle, math::Vec3, float, float, float>{oak, {-8.0f, -1.0f, 24.0f}, 1.25f, 35.0f, 2.20f},
        std::tuple<MeshHandle, math::Vec3, float, float, float>{pine, {8.0f, -1.0f, 24.0f}, 1.20f, -40.0f, 2.75f}}) {
        auto tree = object("Perimeter tree", mesh, vertex_colored_trees, position, {scale, scale, scale},
            {0.0f, math::radians(rotation), 0.0f});
        tree.double_sided = true;
        tree.editor_group = "Perimeter trees";
        tree.editor_pick_offset = {0.0f, pick_height * scale, 0.0f};
        tree.editor_pick_radius_scale = 2.4f;
        scene.objects.push_back(std::move(tree));
    }
    scene.objects.push_back(object("Pool rock cluster A", rocks, rock,
        {22.4f, -0.42f, 4.4f}, {1.20f, 1.20f, 1.20f},
        {0.0f, math::radians(-22.0f), 0.0f}));
    scene.objects.push_back(object("Pool rock cluster B", rocks, rock,
        {24.0f, -0.46f, 5.3f}, {0.92f, 0.92f, 0.92f},
        {0.0f, math::radians(34.0f), 0.0f}));
    scene.objects.push_back(object("Lake rock cluster A", rocks, rock,
        {-18.4f, -0.58f, -27.2f}, {1.28f, 1.28f, 1.28f},
        {0.0f, math::radians(18.0f), 0.0f}));
    scene.objects.push_back(object("Lake rock cluster B", rocks, rock,
        {-16.6f, -0.54f, -28.4f}, {0.96f, 0.96f, 0.96f},
        {0.0f, math::radians(-36.0f), 0.0f}));
    scene.objects.push_back(object("Distant rock cluster", rocks, rock,
        {18.6f, -0.50f, -26.4f}, {1.10f, 1.10f, 1.10f},
        {0.0f, math::radians(-28.0f), 0.0f}));

    // Editor grouping turns multi-part authored props into coherent selectable units.
    for (auto& editable : scene.objects) {
        if (editable.name == "Unified courtyard platform"
            || editable.name.find("retaining face") != std::string::npos) {
            editable.editor_group = "Courtyard platform";
        } else if (editable.name.find("Campfire stone ring") != std::string::npos
                   || editable.name.find("Campfire crossed log") != std::string::npos
                   || editable.name == "Campfire emissive core") {
            editable.editor_group = "Campfire pit";
        } else if (editable.name.find("Reflecting pool") != std::string::npos
                   || editable.name.find("Pool fountain") != std::string::npos
                   || editable.name.find("Pool brushed metal") != std::string::npos) {
            editable.editor_group = "Architectural pool";
        } else if (editable.name == "Flag pole base" || editable.name == "Flag pole"
                   || editable.name == "Flag pole halyard" || editable.name == "Flag pole finial") {
            editable.editor_group = "American flag assembly";
        } else if (editable.name.find("Unity flag pole") != std::string::npos) {
            editable.editor_group = "Israeli flag assembly";
        } else if (editable.name == "Awning support beam") {
            editable.editor_group = "Entrance awning";
        } else if (editable.name.find("Ceiling PTZ") != std::string::npos) {
            editable.editor_group = "Ceiling PTZ camera";
        } else if (editable.name.find("Outdoor security-camera") != std::string::npos) {
            editable.editor_group = "Outdoor box camera";
        } else if (editable.name.find("Pool rock cluster") != std::string::npos) {
            editable.editor_group = "Pool rocks";
        } else if (editable.name.find("Lake rock cluster") != std::string::npos) {
            editable.editor_group = "Lake rocks";
        } else if (editable.name.find("Distant rock cluster") != std::string::npos) {
            editable.editor_group = "Distant rocks";
        } else if (editable.name == "Camera-attached player humanoid") {
            editable.editor_group = "Player model";
        }
        editable.editor_min_scale = {
            std::max(0.01f, std::abs(editable.authored_size.x) * 0.02f),
            std::max(0.01f, std::abs(editable.authored_size.y) * 0.02f),
            std::max(0.01f, std::abs(editable.authored_size.z) * 0.02f)
        };
        editable.editor_max_scale = {
            std::max(2.0f, std::abs(editable.authored_size.x) * 4.0f),
            std::max(2.0f, std::abs(editable.authored_size.y) * 4.0f),
            std::max(2.0f, std::abs(editable.authored_size.z) * 4.0f)
        };
        editable.editor_bounds_min = editable.authored_position - math::Vec3{50.0f, 20.0f, 50.0f};
        editable.editor_bounds_max = editable.authored_position + math::Vec3{50.0f, 35.0f, 50.0f};
    }

    // Editor-owned effect data keeps terrain, water, particles and foliage editable
    // without turning renderer-specific internals into ordinary PBR meshes.
    scene.terrain = {
        .name = "Main terrain",
        .editor_group = "Terrain",
        .transform = {{0.0f, -0.82f, -3.0f}, {}, {32.0f, 1.0f, 32.0f}},
        .height_scale = 3.6f,
        .preset_mask = world_mask,
        .visible = true
    };
    scene.foliage_regions.push_back({
        .name = "Main terrain foliage region",
        .editor_group = "Terrain foliage",
        .transform = {{0.0f, -0.56f, -2.0f}, {}, {29.0f, 1.0f, 26.0f}},
        .blade_scale = {1.0f, 1.0f},
        .density = 1.0f,
        .sway = 1.0f,
        .preset_mask = world_mask
    });
    scene.particle_emitters.push_back({
        .name = "Campfire particle emitter",
        .editor_group = "Campfire pit",
        .transform = {fire_center, {}, {1.0f, 1.0f, 1.0f}},
        .intensity = 1.0f,
        .size_scale = 1.0f,
        .kind = scene::ParticleEmitterKind::fire,
        .preset_mask = world_mask
    });
    scene.particle_emitters.push_back({
        .name = "Pool light-mote emitter",
        .editor_group = "Architectural pool",
        .transform = {{16.0f, 0.18f, 5.0f}, {}, {1.0f, 1.0f, 1.0f}},
        .intensity = 1.0f,
        .size_scale = 1.0f,
        .kind = scene::ParticleEmitterKind::motes,
        .preset_mask = world_mask
    });
    scene.particle_emitters.push_back({
        .name = "Pool mist emitter",
        .editor_group = "Architectural pool",
        .transform = {{16.0f, 0.12f, 5.0f}, {}, {1.0f, 1.0f, 1.0f}},
        .intensity = 1.0f,
        .size_scale = 1.0f,
        .kind = scene::ParticleEmitterKind::mist,
        .preset_mask = world_mask
    });

    const auto add_editor_proxy = [&](std::string name, std::string group,
                                      scene::EditorBindingKind binding, std::size_t binding_index,
                                      math::Vec3 position, math::Vec3 scale,
                                      math::Vec3 minimum_scale, math::Vec3 maximum_scale,
                                      bool lock_scale_y, std::array<float, 4> properties) {
        auto proxy = object(std::move(name), cube, neutral, position, scale);
        proxy.editor_group = std::move(group);
        proxy.editor_binding = binding;
        proxy.editor_binding_index = binding_index;
        proxy.editor_properties = properties;
        proxy.editor_only = true;
        proxy.editor_lock_scale_y = lock_scale_y;
        proxy.casts_shadow = false;
        proxy.receives_shadow = false;
        proxy.editor_min_scale = minimum_scale;
        proxy.editor_max_scale = maximum_scale;
        proxy.editor_bounds_min = {-80.0f, -8.0f, -80.0f};
        proxy.editor_bounds_max = {80.0f, 45.0f, 80.0f};
        scene.objects.push_back(std::move(proxy));
    };

    for (std::size_t index = 0; index < scene.water_surfaces.size(); ++index) {
        const auto& water = scene.water_surfaces[index];
        add_editor_proxy("[Effect] " + water.name, water.editor_group,
            scene::EditorBindingKind::water_surface, index,
            water.transform.position, water.transform.scale,
            {0.25f, 1.0f, 0.25f}, {60.0f, 1.0f, 60.0f}, true,
            {water.wave_amplitude, water.wave_speed, water.roughness, water.opacity});
    }
    for (std::size_t index = 0; index < scene.cloth_objects.size(); ++index) {
        const auto& cloth = scene.cloth_objects[index];
        const std::string cloth_group = cloth.name.find("American") != std::string::npos
            ? "American flag assembly"
            : (cloth.name.find("Israeli") != std::string::npos
                ? "Israeli flag assembly"
                : (cloth.name.find("awning") != std::string::npos ? "Entrance awning" : cloth.name));
        add_editor_proxy("[Effect] " + cloth.name, cloth_group,
            scene::EditorBindingKind::cloth_object, index,
            cloth.origin, {cloth.width, cloth.height, 0.25f},
            {0.25f, 0.25f, 0.10f}, {20.0f, 12.0f, 2.0f}, false,
            {cloth.wind_response, cloth.gravity_scale, cloth.initial_billow, 0.0f});
    }
    for (std::size_t index = 0; index < scene.rtt_displays.size(); ++index) {
        const auto& display = scene.rtt_displays[index];
        add_editor_proxy("[Effect] " + display.name, display.editor_group,
            scene::EditorBindingKind::rtt_display, index,
            display.transform.position, display.transform.scale,
            {0.20f, 0.20f, 0.20f}, {20.0f, 20.0f, 4.0f}, false, {});
    }
    for (std::size_t index = 0; index < scene.point_light_count; ++index) {
        const auto& light = scene.point_lights[index];
        add_editor_proxy("[Effect] Point light " + std::to_string(index + 1u), "Point lights",
            scene::EditorBindingKind::point_light, index,
            light.position, {light.radius * 0.25f, light.radius * 0.25f, light.radius * 0.25f},
            {0.25f, 0.25f, 0.25f}, {12.0f, 12.0f, 12.0f}, false,
            {light.radius, light.intensity, 0.0f, 0.0f});
    }
    add_editor_proxy("[Effect] Red dual-sided spotlight", "Camera room spotlights",
        scene::EditorBindingKind::spotlight, 0u,
        scene.spotlight.position, {scene.spotlight.range * 0.15f, 1.0f, 1.0f},
        {0.25f, 0.25f, 0.25f}, {12.0f, 12.0f, 12.0f}, false,
        {scene.spotlight.range, scene.spotlight.intensity, scene.spotlight.inner_cosine, scene.spotlight.outer_cosine});
    scene.objects.back().transform.rotation = direction_to_editor_rotation(scene.spotlight.authored_direction);

    add_editor_proxy("[Effect] Blue dual-sided spotlight", "Camera room spotlights",
        scene::EditorBindingKind::spotlight, 1u,
        scene.secondary_spotlight.position, {scene.secondary_spotlight.range * 0.15f, 1.0f, 1.0f},
        {0.25f, 0.25f, 0.25f}, {12.0f, 12.0f, 12.0f}, false,
        {scene.secondary_spotlight.range, scene.secondary_spotlight.intensity,
         scene.secondary_spotlight.inner_cosine, scene.secondary_spotlight.outer_cosine});
    scene.objects.back().transform.rotation = direction_to_editor_rotation(
        scene.secondary_spotlight.authored_direction);
    for (std::size_t index = 0; index < scene.foliage_regions.size(); ++index) {
        const auto& region = scene.foliage_regions[index];
        add_editor_proxy("[Effect] " + region.name, region.editor_group,
            scene::EditorBindingKind::foliage_region, index,
            region.transform.position, region.transform.scale,
            {1.0f, 1.0f, 1.0f}, {80.0f, 1.0f, 80.0f}, true,
            {region.density, region.blade_scale.x, region.blade_scale.y, region.sway});
    }
    for (std::size_t index = 0; index < scene.particle_emitters.size(); ++index) {
        const auto& emitter = scene.particle_emitters[index];
        add_editor_proxy("[Effect] " + emitter.name, emitter.editor_group,
            scene::EditorBindingKind::particle_emitter, index,
            emitter.transform.position, {emitter.size_scale, emitter.size_scale, emitter.size_scale},
            {0.10f, 0.10f, 0.10f}, {6.0f, 6.0f, 6.0f}, false,
            {emitter.intensity, emitter.size_scale, 0.0f, 0.0f});
    }

    // Descriptive labels name actual materials and systems. Room spacing and short
    // line lengths keep them readable instead of forming a wall of overlapping text.
    scene.labels.push_back({"METALLIC + POINT-SHADOW ROOM\nBrushed aluminum + specular highlights\nOne 512px cubemap shadow, six Tier-0 passes", {-11.25f, 6.82f, -8.35f}, {0.82f, 0.90f, 1.0f, 1.0f}, 52.0f, world_mask});
    scene.labels.push_back({"PARALLAX BRICK ROOM\nHeight-mapped mortar + normal mapping\nProjected spotlight + PCF sun shadows", {-3.75f, 6.82f, -8.35f}, {1.0f, 0.78f, 0.54f, 1.0f}, 52.0f, world_mask});
    scene.labels.push_back({"RENDER-TO-TEXTURE ROOM\nSecurity camera + overhead site camera\nTwo independent color/depth framebuffers", {3.75f, 6.82f, -8.35f}, {0.58f, 0.84f, 1.0f, 1.0f}, 52.0f, world_mask});
    scene.labels.push_back({"TRANSMISSION ROOM\nFrosted glass + environment refraction\nCarbon fiber + automotive clearcoat", {11.25f, 6.82f, -8.35f}, {0.92f, 0.72f, 0.62f, 1.0f}, 52.0f, world_mask});
    scene.labels.push_back({"PLANAR MIRROR RTT\nReflected main camera + standard FBO\nNo geometry shader or vendor extension", {5.5f, 4.0f, 8.4f}, {0.72f, 0.86f, 1.0f, 1.0f}, 46.0f, world_mask});
    scene.labels.push_back({"CAMPFIRE SEATING\nParticles + bloom + animated local light", {-15.0f, 2.15f, 8.0f}, {1.0f, 0.64f, 0.28f, 1.0f}, 46.0f, world_mask});
    scene.labels.push_back({"CLOTH SIMULATION\nAmerican + Israeli flags and woven awning\nFixed-step constraints + dynamic normals", {-18.3f, 7.15f, -3.2f}, {0.82f, 0.90f, 1.0f, 1.0f}, 48.0f, world_mask});
    scene.labels.push_back({"INSTANCED PROPS\n12 crates in one draw\nTier 0 instancing / optional indirect draw", {18.6f, 2.65f, -10.0f}, {0.88f, 0.82f, 0.66f, 1.0f}, 46.0f, world_mask});
    scene.labels.push_back({"ARCHITECTURAL WATER\nBordered pool + scene-color refraction\nFresnel reflection + low-slope waves", {16.0f, 2.25f, 5.0f}, {0.54f, 0.86f, 0.96f, 1.0f}, 48.0f, world_mask});
    scene.labels.push_back({"TIER-0 TERRAIN\nSplat blending + meter-scaled textures\nVertex heightfield + instanced foliage", {0.0f, 1.25f, 22.0f}, {0.70f, 0.86f, 0.58f, 1.0f}, 48.0f, world_mask});
    scene.labels.push_back({"DISTANT TERRAIN WATER\nLow-cost pond surface beyond the campus", {0.0f, 0.85f, -30.0f}, {0.54f, 0.76f, 0.94f, 1.0f}, 44.0f, world_mask});
    scene.labels.push_back({"VERTEX-EXPANDED FOLIAGE\nMobile/Switch billboard path\nNo geometry shader required", {25.0f, 1.4f, 12.0f}, {0.64f, 0.86f, 0.54f, 1.0f}, 44.0f, world_mask});

    // Object-local prompt labels map visible results directly to the GUI control that
    // owns them. Short local ranges keep the overlay technical without filling the
    // entire horizon with unrelated labels.
    scene.labels.push_back({"BRUSHED ALUMINUM TORUS\nLighting > Cubemap ambient and reflections\nLighting > Local point lights", {-11.25f, 4.55f, -14.0f}, {0.66f, 0.84f, 1.0f, 1.0f}, 30.0f, world_mask});
    scene.labels.push_back({"POINT-SHADOW OCCLUDERS\nLighting > Point-light cubemap shadow\nSix depth faces around the nearest light", {-11.25f, 4.65f, -16.55f}, {1.0f, 0.70f, 0.42f, 1.0f}, 30.0f, world_mask});
    scene.labels.push_back({"AUTOMOTIVE CLEARCOAT\nLighting > Clearcoat response\nTuning > Clearcoat strength", {-13.2f, 3.35f, -12.25f}, {1.0f, 0.60f, 0.52f, 1.0f}, 26.0f, world_mask});
    scene.labels.push_back({"PARALLAX RECEIVER\nLighting > Parallax occlusion mapping\nTuning > Parallax depth", {-3.75f, 3.05f, -13.60f}, {1.0f, 0.76f, 0.42f, 1.0f}, 26.0f, world_mask});
    scene.labels.push_back({"PROJECTED COOKIE\nLighting > Projected spotlight\nLighting > Projected texture cookie", {-3.75f, 4.15f, -16.25f}, {1.0f, 0.82f, 0.52f, 1.0f}, 30.0f, world_mask});
    scene.labels.push_back({"SECURITY-CAMERA RTT\nTier 0 > Live camera render targets", {2.15f, 3.15f, -18.70f}, {0.52f, 0.84f, 1.0f, 1.0f}, 24.0f, world_mask});
    scene.labels.push_back({"OVERHEAD-CAMERA RTT\nTier 0 > Live camera render targets", {5.30f, 3.15f, -18.70f}, {0.52f, 0.84f, 1.0f, 1.0f}, 24.0f, world_mask});
    scene.labels.push_back({"CEILING-CAMERA RTT\nSeparate upper display and FBO", {3.73f, 4.80f, -18.70f}, {0.52f, 0.84f, 1.0f, 1.0f}, 28.0f, world_mask});
    scene.labels.push_back({"CEILING PTZ CAMERA\nAimed from the actual RTT transform", {1.22f, 5.92f, -9.60f}, {0.72f, 0.88f, 1.0f, 1.0f}, 24.0f, world_mask});
    scene.labels.push_back({"OUTDOOR BOX CAMERA\nOriginal lower security feed", {-15.0f, 7.25f, 12.5f}, {0.72f, 0.88f, 1.0f, 1.0f}, 30.0f, world_mask});
    scene.labels.push_back({"FROSTED TRANSMISSION\nLighting > Transmission and refraction\nTuning > Transmission strength", {9.55f, 3.75f, -14.0f}, {0.72f, 0.90f, 1.0f, 1.0f}, 26.0f, world_mask});
    scene.labels.push_back({"CAMPFIRE PARTICLES + BLOOM\nTier 0 > GPU particle fire / HDR bloom\nTuning > Bloom strength and threshold", {-15.0f, 2.55f, 8.0f}, {1.0f, 0.56f, 0.22f, 1.0f}, 24.0f, world_mask});
    scene.labels.push_back({"CONNECTED BENCH ASSEMBLY\nLighting > Normal mapping\nTuning > Bench exploded view", {-15.0f, 2.35f, 10.75f}, {0.84f, 0.70f, 0.48f, 1.0f}, 28.0f, world_mask});
    scene.labels.push_back({"CLOTH FLAGS\nTier 0 > Constraint cloth and flag\nTuning > Cloth wind", {0.0f, 7.25f, -3.5f}, {0.72f, 0.86f, 1.0f, 1.0f}, 34.0f, world_mask});
    scene.labels.push_back({"PLANAR MIRROR FEED\nTier 0 > One-sided planar mirror RTT", {5.5f, 3.35f, 8.30f}, {0.66f, 0.86f, 1.0f, 1.0f}, 24.0f, world_mask});
    scene.labels.push_back({"POOL WATER\nTier 0 > Depth-aware water surfaces\nTuning > Water wave / refraction", {16.0f, 1.55f, 5.0f}, {0.50f, 0.88f, 1.0f, 1.0f}, 28.0f, world_mask});
    scene.labels.push_back({"SSAO CONTACT SHADOWS\nTier 1 > Screen-space ambient occlusion\nTuning > SSAO strength / radius", {0.0f, 1.05f, -8.9f}, {0.70f, 0.78f, 0.92f, 1.0f}, 26.0f, world_mask});
    scene.labels.push_back({"TESSELLATED TERRAIN\nTier 1 > Hardware terrain tessellation\nTuning > Tessellation level", {0.0f, 1.35f, 18.0f}, {0.70f, 0.90f, 0.56f, 1.0f}, 28.0f, world_mask});
    scene.labels.push_back({"INSTANCED FOLIAGE\nTier 0 > Instanced vertex foliage\nTuning > Foliage density", {22.5f, 1.65f, 12.0f}, {0.62f, 0.90f, 0.54f, 1.0f}, 26.0f, world_mask});

    for (std::size_t index = 0; index < scene.labels.size(); ++index) {
        const auto& label = scene.labels[index];
        add_editor_proxy("[Effect] Scene label " + std::to_string(index + 1u), "",
            scene::EditorBindingKind::scene_label, index,
            label.world_position, {0.65f, 0.65f, 0.65f},
            {0.20f, 0.20f, 0.20f}, {3.0f, 3.0f, 3.0f}, false, {});
    }

    // Diagnostics remain a separate preset and never clutter the integrated scene.
    scene.objects.push_back(object("Hard-edge diagnostic", hard_edges, neutral,
        {-3.0f, 1.0f, 14.0f}, {1.25f, 1.25f, 1.25f}, {}, diagnostics_mask));
    scene.objects.push_back(object("Smooth-edge diagnostic", smooth_edges, ceramic,
        {0.0f, 1.0f, 14.0f}, {1.25f, 1.25f, 1.25f}, {}, diagnostics_mask));
    scene.objects.push_back(object("Atlas diagnostic", atlas_cube, neutral,
        {3.0f, 1.0f, 14.0f}, {1.25f, 1.25f, 1.25f}, {}, diagnostics_mask));

    // Freestanding support for the outdoor security camera. It is appended after
    // every config-indexed entity so existing saved V2 indices remain stable.
    auto outdoor_camera_pole = object("Outdoor security-camera support pole", cube, aluminum,
        {-15.7448425f, 3.06f, 12.6831579f}, {0.18f, 7.24f, 0.18f});
    outdoor_camera_pole.editor_group = "Outdoor box camera";
    outdoor_camera_pole.editor_min_scale = {0.05f, 0.50f, 0.05f};
    outdoor_camera_pole.editor_max_scale = {1.50f, 14.0f, 1.50f};
    scene.objects.push_back(std::move(outdoor_camera_pole));

    const auto add_dual_spotlight_fixture = [&](std::string_view color_name,
                                                const scene::SpotLight& light,
                                                MaterialHandle emissive_material) {
        const math::Vec3 rotation = camera_euler(light.position, light.position + light.authored_direction);
        auto mount = object(std::string(color_name) + " spotlight ceiling mount", cylinder, aluminum,
            light.position + math::Vec3{0.0f, 0.34f, 0.0f}, {0.26f, 0.12f, 0.26f});
        mount.editor_group = "Camera room spotlights";
        scene.objects.push_back(std::move(mount));

        auto yoke = object(std::string(color_name) + " spotlight rotating yoke", cube, carbon,
            light.position, {0.48f, 0.16f, 0.16f}, rotation);
        yoke.editor_group = "Camera room spotlights";
        scene.objects.push_back(std::move(yoke));

        auto lamp = object(std::string(color_name) + " dual-sided spotlight lamp", cylinder, emissive_material,
            light.position, {0.20f, 0.62f, 0.20f},
            {rotation.x + math::radians(90.0f), rotation.y, rotation.z});
        const std::size_t light_index = color_name == "Red" ? 0u : 1u;
        lamp.editor_group = std::string(color_name) + " camera-room spotlight";
        lamp.editor_binding = scene::EditorBindingKind::spotlight;
        lamp.editor_binding_index = light_index;
        lamp.editor_properties = {light.range, light.intensity, light.inner_cosine, light.outer_cosine};
        lamp.double_sided = true;
        lamp.casts_shadow = false;
        scene.objects.push_back(std::move(lamp));
    };
    add_dual_spotlight_fixture("Red", scene.spotlight, emissive_red);
    add_dual_spotlight_fixture("Blue", scene.secondary_spotlight, emissive_blue);

    // Keep the cfg11 object order stable by appending the restored projector after
    // the existing 244 authored entries.
    add_editor_proxy("[Effect] Projector cookie spotlight", "Projected cookie spotlight",
        scene::EditorBindingKind::spotlight, 2u,
        scene.projector_spotlight.position,
        {scene.projector_spotlight.range * 0.15f, 1.0f, 1.0f},
        {0.25f, 0.25f, 0.25f}, {12.0f, 12.0f, 12.0f}, false,
        {scene.projector_spotlight.range, scene.projector_spotlight.intensity,
         scene.projector_spotlight.inner_cosine, scene.projector_spotlight.outer_cosine});
    scene.objects.back().transform.rotation = direction_to_editor_rotation(
        scene.projector_spotlight.authored_direction);

    const math::Vec3 projector_rotation = camera_euler(
        scene.projector_spotlight.position,
        scene.projector_spotlight.position + scene.projector_spotlight.authored_direction);
    auto projector_mount = object("Projector spotlight ceiling mount", cylinder, aluminum,
        scene.projector_spotlight.position + math::Vec3{0.0f, 0.34f, 0.0f},
        {0.26f, 0.12f, 0.26f});
    projector_mount.editor_group = "Projected cookie spotlight";
    scene.objects.push_back(std::move(projector_mount));

    auto projector_yoke = object("Projector spotlight rotating yoke", cube, carbon,
        scene.projector_spotlight.position, {0.48f, 0.16f, 0.16f}, projector_rotation);
    projector_yoke.editor_group = "Projected cookie spotlight";
    scene.objects.push_back(std::move(projector_yoke));

    auto projector_lamp = object("Projector cookie spotlight lamp", cylinder, emissive_warm,
        scene.projector_spotlight.position, {0.20f, 0.62f, 0.20f},
        {projector_rotation.x + math::radians(90.0f), projector_rotation.y, projector_rotation.z});
    projector_lamp.editor_group = "Projected cookie spotlight";
    projector_lamp.editor_binding = scene::EditorBindingKind::spotlight;
    projector_lamp.editor_binding_index = 2u;
    projector_lamp.editor_properties = {
        scene.projector_spotlight.range, scene.projector_spotlight.intensity,
        scene.projector_spotlight.inner_cosine, scene.projector_spotlight.outer_cosine};
    projector_lamp.double_sided = true;
    projector_lamp.casts_shadow = false;
    scene.objects.push_back(std::move(projector_lamp));

    constexpr math::Vec3 twin_pool_offset{12.5f, 0.0f, 0.0f};
    constexpr math::Vec3 twin_pool_center = pool_center + twin_pool_offset;

    auto twin_pool_bed = object("Twin reflecting pool submerged bed", cube, waterbed,
        twin_pool_center + math::Vec3{0.0f, -0.30f, 0.0f}, {11.0f, 0.34f, 6.5f});
    twin_pool_bed.editor_group = "Architectural pool duplicate";
    scene.objects.push_back(std::move(twin_pool_bed));
    auto twin_pool_north = object("Twin reflecting pool north coping", cube, concrete,
        twin_pool_center + math::Vec3{0.0f, 0.08f, -3.55f}, {11.8f, 0.38f, 0.42f});
    twin_pool_north.editor_group = "Architectural pool duplicate";
    scene.objects.push_back(std::move(twin_pool_north));
    auto twin_pool_south = object("Twin reflecting pool south coping", cube, concrete,
        twin_pool_center + math::Vec3{0.0f, 0.08f, 3.55f}, {11.8f, 0.38f, 0.42f});
    twin_pool_south.editor_group = "Architectural pool duplicate";
    scene.objects.push_back(std::move(twin_pool_south));
    auto twin_pool_west = object("Twin reflecting pool west coping", cube, concrete,
        twin_pool_center + math::Vec3{-5.70f, 0.08f, 0.0f}, {0.42f, 0.38f, 6.7f});
    twin_pool_west.editor_group = "Architectural pool duplicate";
    scene.objects.push_back(std::move(twin_pool_west));
    auto twin_pool_east = object("Twin reflecting pool east coping", cube, concrete,
        twin_pool_center + math::Vec3{5.70f, 0.08f, 0.0f}, {0.42f, 0.38f, 6.7f});
    twin_pool_east.editor_group = "Architectural pool duplicate";
    scene.objects.push_back(std::move(twin_pool_east));
    auto twin_pool_plinth = object("Twin pool fountain plinth", cylinder, concrete,
        twin_pool_center + math::Vec3{0.0f, 0.22f, 0.0f}, {0.85f, 0.50f, 0.85f});
    twin_pool_plinth.editor_group = "Architectural pool duplicate";
    scene.objects.push_back(std::move(twin_pool_plinth));
    auto twin_pool_sculpture = object("Twin pool brushed metal sculpture", torus, aluminum,
        twin_pool_center + math::Vec3{0.0f, 1.45f, 0.0f}, {1.75f, 1.75f, 1.75f});
    twin_pool_sculpture.editor_group = "Architectural pool duplicate";
    scene.objects.push_back(std::move(twin_pool_sculpture));
    auto twin_pool_rock_a = object("Twin pool rock cluster A", rocks, rock,
        math::Vec3{22.4f, -0.42f, 4.4f} + twin_pool_offset, {1.20f, 1.20f, 1.20f},
        {0.0f, math::radians(-22.0f), 0.0f});
    twin_pool_rock_a.editor_group = "Twin pool rocks";
    scene.objects.push_back(std::move(twin_pool_rock_a));
    auto twin_pool_rock_b = object("Twin pool rock cluster B", rocks, rock,
        math::Vec3{24.0f, -0.46f, 5.3f} + twin_pool_offset, {0.92f, 0.92f, 0.92f},
        {0.0f, math::radians(34.0f), 0.0f});
    twin_pool_rock_b.editor_group = "Twin pool rocks";
    scene.objects.push_back(std::move(twin_pool_rock_b));

    scene.water_surfaces.push_back({
        .name = "Twin bordered architectural reflecting pool",
        .editor_group = "Architectural pool duplicate",
        .transform = {twin_pool_center + math::Vec3{0.0f, 0.02f, 0.0f}, {}, {5.45f, 1.0f, 3.15f}},
        .shallow_color = {0.085f, 0.28f, 0.32f},
        .deep_color = {0.018f, 0.065f, 0.090f},
        .flow_direction = {0.40f, 1.0f},
        .wave_amplitude = 0.050f,
        .wave_speed = 0.58f,
        .roughness = 0.060f,
        .opacity = 0.91f,
        .foam_strength = 0.34f,
        .kind = scene::WaterSurfaceKind::pool,
        .preset_mask = world_mask
    });
    scene.particle_emitters.push_back({
        .name = "Twin pool light-mote emitter",
        .editor_group = "Architectural pool duplicate",
        .transform = {twin_pool_center + math::Vec3{0.0f, 0.18f, 0.0f}, {}, {1.0f, 1.0f, 1.0f}},
        .intensity = 1.0f,
        .size_scale = 1.0f,
        .kind = scene::ParticleEmitterKind::motes,
        .preset_mask = world_mask
    });
    scene.particle_emitters.push_back({
        .name = "Twin pool mist emitter",
        .editor_group = "Architectural pool duplicate",
        .transform = {twin_pool_center + math::Vec3{0.0f, 0.12f, 0.0f}, {}, {1.0f, 1.0f, 1.0f}},
        .intensity = 1.0f,
        .size_scale = 1.0f,
        .kind = scene::ParticleEmitterKind::mist,
        .preset_mask = world_mask
    });

    add_editor_proxy("[Effect] Twin bordered architectural reflecting pool", "Architectural pool duplicate",
        scene::EditorBindingKind::water_surface, scene.water_surfaces.size() - 1u,
        twin_pool_center + math::Vec3{0.0f, 0.02f, 0.0f}, {5.45f, 1.0f, 3.15f},
        {0.25f, 1.0f, 0.25f}, {60.0f, 1.0f, 60.0f}, true,
        {0.050f, 0.58f, 0.060f, 0.91f});
    add_editor_proxy("[Effect] Twin pool light-mote emitter", "Architectural pool duplicate",
        scene::EditorBindingKind::particle_emitter, scene.particle_emitters.size() - 2u,
        twin_pool_center + math::Vec3{0.0f, 0.18f, 0.0f}, {1.0f, 1.0f, 1.0f},
        {0.10f, 0.10f, 0.10f}, {6.0f, 6.0f, 6.0f}, false,
        {1.0f, 1.0f, 0.0f, 0.0f});
    add_editor_proxy("[Effect] Twin pool mist emitter", "Architectural pool duplicate",
        scene::EditorBindingKind::particle_emitter, scene.particle_emitters.size() - 1u,
        twin_pool_center + math::Vec3{0.0f, 0.12f, 0.0f}, {1.0f, 1.0f, 1.0f},
        {0.10f, 0.10f, 0.10f}, {6.0f, 6.0f, 6.0f}, false,
        {1.0f, 1.0f, 0.0f, 0.0f});

    // Each item remains one instance in one instanced draw call, but is represented
    // by a normal selectable scene object so transform, Enabled and Delete work.
    for (std::size_t index = 0; index < 12u; ++index) {
        const int column = static_cast<int>(index % 4u);
        const int row = static_cast<int>(index / 4u);
        auto crate_instance = object(
            "Instanced crate " + std::to_string(index + 1u), crate, wood,
            {16.95f + static_cast<float>(column) * 1.10f, 0.62f,
             -8.65f - static_cast<float>(row) * 1.12f},
            {0.72f, 0.72f, 0.72f}, {0.0f, static_cast<float>(index) * 0.37f, 0.0f});
        crate_instance.editor_group = "Instanced crate draw";
        crate_instance.editor_binding = scene::EditorBindingKind::instanced_prop;
        crate_instance.editor_binding_index = index;
        crate_instance.casts_shadow = false;
        crate_instance.editor_pick_offset = {0.0f, 0.45f, 0.0f};
        crate_instance.editor_pick_radius_scale = 1.6f;
        scene.objects.push_back(std::move(crate_instance));
    }


    // Terrain is a first-class editor object. It remains renderer-owned, but can be
    // selected directly, moved, resized, enabled/deleted, and have its displacement
    // height edited without creating a second visible proxy mesh.
    add_editor_proxy("Main terrain", "Terrain",
        scene::EditorBindingKind::terrain_surface, 0u,
        scene.terrain.transform.position, scene.terrain.transform.scale,
        {4.0f, 0.05f, 4.0f}, {128.0f, 8.0f, 128.0f}, false,
        {scene.terrain.height_scale, 0.0f, 0.0f, 0.0f});
    scene.objects.back().editor_pick_offset = {0.0f, 0.5f, 0.0f};
    scene.objects.back().editor_pick_radius_scale = 1.0f;
    scene.objects.back().editor_bounds_min = {-160.0f, -40.0f, -160.0f};
    scene.objects.back().editor_bounds_max = {160.0f, 40.0f, 160.0f};

    // Authored defaults are applied by RenderSpineImpl after optional editor models load.
}

} // namespace epoch::render::world
