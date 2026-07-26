#include "epoch/render/detail/render_spine_impl.hpp"
#include "epoch/render/gl/gl_api.hpp"
#include "epoch/render/world/world_scene.hpp"
#include <algorithm>
#include <cmath>
#include <cwctype>
#include <iterator>
#include <filesystem>
#include <functional>
#include <stdexcept>
#include <string>
#include <string_view>

#if defined(_WIN32)
#include <windows.h>
#include <commdlg.h>
#endif


#if defined(EPOCH_NO_MODULES)
#include "epoch/compat/epoch.core.log.hpp"
#else
import epoch.core.log;
#endif
namespace epoch::render::detail {
namespace {
inline constexpr GLenum gl_major_version = 0x821B;
inline constexpr GLenum gl_minor_version = 0x821C;
inline constexpr GLenum gl_max_texture_size = 0x0D33;

void APIENTRY debug_callback(GLenum, GLenum, GLuint, GLenum severity, GLsizei, const gl::GLchar* message, const void*) {
    if (severity == 0x826B) return; // notification
    core::log_error(message ? message : "OpenGL debug callback without message");
}

std::filesystem::path choose_editor_file(gui::GuiOverlay::EditorImportRequest request) {
#if defined(_WIN32)
    wchar_t buffer[32768]{};
    const wchar_t* filter = request == gui::GuiOverlay::EditorImportRequest::obj_model
        ? L"Wavefront OBJ (*.obj)\0*.obj\0All files (*.*)\0*.*\0\0"
        : L"Image files (*.png;*.jpg;*.jpeg;*.bmp;*.tif;*.tiff)\0*.png;*.jpg;*.jpeg;*.bmp;*.tif;*.tiff\0All files (*.*)\0*.*\0\0";
    OPENFILENAMEW dialog{};
    dialog.lStructSize = sizeof(dialog);
    dialog.lpstrFile = buffer;
    dialog.nMaxFile = static_cast<DWORD>(std::size(buffer));
    dialog.lpstrFilter = filter;
    dialog.nFilterIndex = 1;
    dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    dialog.lpstrTitle = request == gui::GuiOverlay::EditorImportRequest::obj_model
        ? L"Load model into Epoch scene editor"
        : L"Load texture into Epoch scene editor";
    if (GetOpenFileNameW(&dialog)) return std::filesystem::path{buffer};
#endif
    return {};
}

std::string imported_display_name(const std::filesystem::path& staged_path) {
    std::string stem = staged_path.stem().string();
    const auto separator = stem.rfind('_');
    if (separator != std::string::npos && separator + 1u < stem.size()) {
        const bool numeric_suffix = std::all_of(stem.begin() + static_cast<std::ptrdiff_t>(separator + 1u), stem.end(),
            [](unsigned char value) { return value >= '0' && value <= '9'; });
        if (numeric_suffix) stem.resize(separator);
    }
    return "Imported: " + stem + staged_path.extension().string();
}

std::filesystem::path stage_editor_import(const std::filesystem::path& asset_root,
                                          const std::filesystem::path& source,
                                          std::string_view category) {
    if (source.empty()) return {};
    const auto directory = asset_root / "editor/imports" / category;
    std::error_code error;
    std::filesystem::create_directories(directory, error);
    std::string identity = source.lexically_normal().string();
    const auto write_time = std::filesystem::last_write_time(source, error);
    if (!error) identity += std::to_string(write_time.time_since_epoch().count());
    error.clear();
    const auto suffix = std::to_string(std::hash<std::string>{}(identity));
    const auto target = directory / (source.stem().string() + "_" + suffix + source.extension().string());
    if (std::filesystem::exists(target, error)) return std::filesystem::relative(target, asset_root);
    error.clear();
    std::filesystem::copy_file(source, target, std::filesystem::copy_options::overwrite_existing, error);
    if (error) throw std::runtime_error("Unable to copy editor import: " + error.message());
    return std::filesystem::relative(target, asset_root);
}
}

RenderSpineImpl::RenderSpineImpl(std::filesystem::path asset_root)
    : resources_{asset_root},
      sky_mesh_{resources_.create_mesh(gl::make_cube_mesh())},
      billboard_mesh_{resources_.create_mesh(gl::make_billboard_mesh())},
      instanced_crate_mesh_{resources_.load_obj_mesh("default_pack/models/props/crate.obj")},
      terrain_patch_mesh_{resources_.create_mesh(gl::make_grid_mesh(33, 33, 2.0f, 2.0f, 1.0f))},
      shadows_{asset_root / "shaders"},
      point_shadows_{asset_root / "shaders"},
      sky_{asset_root / "shaders", sky_mesh_},
      pbr_{asset_root / "shaders", resources_},
      billboards_{asset_root / "shaders", resources_, billboard_mesh_},
      particles_{asset_root / "shaders"},
      instanced_props_{asset_root / "shaders", resources_, instanced_crate_mesh_},
      render_to_texture_{asset_root / "shaders", billboard_mesh_},
      tessellation_{asset_root / "shaders", resources_, terrain_patch_mesh_},
      cloth_{asset_root / "shaders", resources_},
      water_{asset_root / "shaders", resources_},
      bloom_{asset_root / "shaders"},
      post_{asset_root / "shaders"},
      gui_{asset_root, resources_} {
    detect_capabilities();
    gl::Enable(GL_DEPTH_TEST); gl::DepthFunc(GL_LESS);
    gl::Enable(GL_CULL_FACE); gl::CullFace(GL_BACK); gl::FrontFace(GL_CCW);
    gl::Enable(gl::multisample);
    gl::Disable(gl::framebuffer_srgb);
    gl::ClearColor(0.012f, 0.018f, 0.032f, 1.0f);
    world::WorldSceneBuilder::build(resources_, scene_);
    (void)world::WorldSceneBuilder::write_authored_default_config(
        resources_.asset_root() / "editor/default_scene.cfg", false);
    const std::filesystem::path imported_models = resources_.asset_root() / "editor/imports/models";
    std::error_code import_error;
    if (std::filesystem::exists(imported_models, import_error)) {
        for (const auto& entry : std::filesystem::directory_iterator(imported_models, import_error)) {
            if (import_error || !entry.is_regular_file()) continue;
            auto extension = entry.path().extension().wstring();
            std::transform(extension.begin(), extension.end(), extension.begin(), [](wchar_t value) { return static_cast<wchar_t>(std::towlower(value)); });
            if (extension != L".obj") continue;
            try {
                const auto relative = std::filesystem::relative(entry.path(), resources_.asset_root());
                const MeshHandle imported = resources_.load_obj_mesh(relative);
                scene_.editor_models.push_back({imported_display_name(entry.path()), imported});
            } catch (const std::exception& error) {
                core::log_error(error.what());
            }
        }
    }
    world::WorldSceneBuilder::apply_authored_defaults(resources_, scene_);
    gui_.initialize_scene_editor(scene_);
}

void RenderSpineImpl::detect_capabilities() {
    gl::GetIntegerv(gl_major_version, &capabilities_.gl_major);
    gl::GetIntegerv(gl_minor_version, &capabilities_.gl_minor);
    gl::GetIntegerv(gl_max_texture_size, &capabilities_.max_texture_size);
    capabilities_.max_anisotropy = resources_.max_anisotropy();
    capabilities_.texture_anisotropy = capabilities_.max_anisotropy > 1.0f;
    capabilities_.debug_output = gl::DebugMessageCallback != nullptr;
    if (capabilities_.gl_major < 4 || (capabilities_.gl_major == 4 && capabilities_.gl_minor < 5))
        throw std::runtime_error("OpenGL 4.5 or newer is required");
#ifndef NDEBUG
    if (gl::DebugMessageCallback) {
        gl::Enable(gl::debug_output); gl::Enable(gl::debug_output_synchronous);
        gl::DebugMessageCallback(debug_callback, nullptr);
    }
#endif
}

void RenderSpineImpl::update(context::InputState& input, const context::FrameContext& frame, context::RuntimeControls& controls) {
    const bool gui_capture = gui_.update(input, frame, controls, scene_);
    const auto import_request = gui_.consume_import_request();
    if (import_request != gui::GuiOverlay::EditorImportRequest::none) {
        try {
            const auto source = choose_editor_file(import_request);
            if (!source.empty()) {
                if (import_request == gui::GuiOverlay::EditorImportRequest::obj_model) {
                    const auto relative = stage_editor_import(resources_.asset_root(), source, "models");
                    const MeshHandle imported = resources_.load_obj_mesh(relative);
                    gui_.apply_imported_mesh(scene_, imported, imported_display_name(source));
                } else {
                    const auto relative = stage_editor_import(resources_.asset_root(), source, "textures");
                    const auto color_space = import_request == gui::GuiOverlay::EditorImportRequest::albedo_texture
                        ? gl::ColorSpace::srgb : gl::ColorSpace::linear;
                    const TextureHandle texture = resources_.load_texture(relative, color_space);
                    gui_.apply_imported_texture(scene_, texture, import_request,
                                                "Imported texture: " + source.filename().string(),
                                                relative.generic_string());
                }
            }
        } catch (const std::exception& error) {
            core::log_error(error.what());
        }
    }
    if (!gui_capture) scene_.camera.update(input, frame.delta_seconds);
    else { input.mouse_dx = 0; input.mouse_dy = 0; }
    scene_.update(frame.elapsed_seconds, frame.delta_seconds, controls);
    gui_.synchronize_scene_editor(scene_);
    cloth_.update(scene_, frame.delta_seconds, frame.elapsed_seconds, controls);
    particles_.update(frame.delta_seconds, frame.elapsed_seconds, controls);
}

techniques::TechniqueContext RenderSpineImpl::make_technique_context(const context::FrameContext& frame,
                                                                  const context::RuntimeControls& controls) const {
    const float aspect = static_cast<float>(frame.framebuffer_width) / static_cast<float>(frame.framebuffer_height);
    techniques::TechniqueContext result;
    result.view = scene_.camera.view();
    result.projection = scene_.camera.projection(aspect);
    result.view_projection = result.projection * result.view;
    const math::Vec3 center{0.0f, 1.6f, -5.0f};
    const math::Vec3 light_position = center - scene_.sun.direction * 42.0f;
    const math::Vec3 world_up{0.0f, 1.0f, 0.0f};
    const math::Vec3 shadow_up = std::abs(math::dot(scene_.sun.direction, world_up)) > 0.94f
        ? math::Vec3{0.0f, 0.0f, 1.0f} : world_up;
    const math::Mat4 light_view = math::look_at(light_position, center, shadow_up);
    const math::Mat4 light_projection = math::ortho(-32.0f, 32.0f, -32.0f, 32.0f, 1.0f, 105.0f);
    result.light_view_projection = light_projection * light_view;
    const auto& projector = scene_.projector_spotlight;
    const math::Vec3 projector_target = projector.position + projector.direction;
    const math::Vec3 projector_up = std::abs(projector.direction.y) > 0.94f
        ? math::Vec3{0,0,1} : math::Vec3{0,1,0};
    result.projector_view_projection = math::perspective(
        math::radians(58.0f), 1.0f, 0.25f, projector.range)
        * math::look_at(projector.position, projector_target, projector_up);
    result.camera_position = scene_.camera.position;
    result.camera_forward = scene_.camera.forward();
    result.camera_right = math::normalize(math::cross(result.camera_forward, math::Vec3{0,1,0}));
    result.camera_up = math::normalize(math::cross(result.camera_right, result.camera_forward));
    result.width = frame.framebuffer_width; result.height = frame.framebuffer_height;
    result.elapsed_seconds = frame.elapsed_seconds;
    result.camera_near = scene_.camera.near_plane;
    result.camera_far = scene_.camera.far_plane;
    result.controls = controls;
    return result;
}


techniques::TechniqueContext RenderSpineImpl::make_feed_context(
    const techniques::TechniqueContext& base,
    math::Vec3 position,
    math::Vec3 forward,
    math::Vec3 up,
    float vertical_fov_degrees) const {
    techniques::TechniqueContext result = base;
    const float aspect = static_cast<float>(render_to_texture_.feed_width())
        / static_cast<float>(render_to_texture_.feed_height());
    result.camera_position = position;
    result.camera_forward = math::normalize(forward);
    result.camera_right = math::normalize(math::cross(result.camera_forward, up));
    result.camera_up = math::normalize(math::cross(result.camera_right, result.camera_forward));
    result.view = math::look_at(position, position + result.camera_forward, result.camera_up);
    result.projection = math::perspective(
        math::radians(vertical_fov_degrees), aspect, scene_.camera.near_plane, scene_.camera.far_plane);
    result.view_projection = result.projection * result.view;
    result.width = render_to_texture_.feed_width();
    result.height = render_to_texture_.feed_height();
    result.controls.scene_debug_view = false;
    result.controls.debug_hidden_objects = false;
    result.controls.debug_effect_bounds = false;
    return result;
}

void RenderSpineImpl::render_world_pass(
    const techniques::TechniqueContext& frame, bool include_transparents) const {
    sky_.render(frame, scene_, resources_);
    gl::PolygonMode(GL_FRONT_AND_BACK, frame.controls.wireframe ? GL_LINE : GL_FILL);
    tessellation_.render(frame, scene_, resources_, shadows_);
    pbr_.render(frame, scene_, resources_, shadows_, point_shadows_);
    billboards_.render(frame, scene_, resources_);
    instanced_props_.render(frame, scene_, resources_);
    cloth_.render(frame, scene_, resources_, shadows_);
    if (include_transparents) particles_.render(frame, scene_);
    gl::PolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void RenderSpineImpl::render_rtt_feeds(const techniques::TechniqueContext& base) {
    if (!base.controls.render_to_texture) return;

    if (base.controls.tier0_mobile_profile)
        render_to_texture_.resize(512, 288);
    else
        render_to_texture_.resize(768, 432);

    const auto& camera = scene_.security_camera;
    const auto security = make_feed_context(
        base,
        camera.position,
        camera.target - camera.position,
        camera.up,
        camera.vertical_fov_degrees);
    render_to_texture_.begin_feed(scene::RttFeed::security_camera);
    render_world_pass(security, false);

    const auto& overhead_camera = scene_.overhead_camera;
    const auto overhead = make_feed_context(
        base,
        overhead_camera.position,
        overhead_camera.target - overhead_camera.position,
        overhead_camera.up,
        overhead_camera.vertical_fov_degrees);
    render_to_texture_.begin_feed(scene::RttFeed::overhead_camera);
    render_world_pass(overhead, false);

    const auto& ceiling_camera = scene_.ceiling_camera;
    const auto ceiling = make_feed_context(
        base,
        ceiling_camera.position,
        ceiling_camera.target - ceiling_camera.position,
        ceiling_camera.up,
        ceiling_camera.vertical_fov_degrees);
    render_to_texture_.begin_feed(scene::RttFeed::ceiling_camera);
    render_world_pass(ceiling, false);

    if (base.controls.mirror_rtt) {
        const math::Vec3 normal = math::normalize(scene_.mirror_plane.normal);
        const math::Vec3 mirror_position = scene_.mirror_plane.center + normal * 0.09f;
        const math::Vec3 mirror_forward = normal;
        const auto mirror = make_feed_context(
            base, mirror_position, mirror_forward, scene_.mirror_plane.up, 58.0f);
        render_to_texture_.begin_feed(scene::RttFeed::planar_mirror);
        render_world_pass(mirror, false);
    }
}

void RenderSpineImpl::render(const context::FrameContext& frame, const context::RuntimeControls& controls) {
    hdr_target_.resize(frame.framebuffer_width, frame.framebuffer_height);
    bloom_.resize(frame.framebuffer_width, frame.framebuffer_height);
    const auto technique_frame = make_technique_context(frame, controls);

    gpu_profiler_.begin(controls.gpu_queries && !controls.tier0_mobile_profile);
    shadows_.render(technique_frame, scene_, resources_);
    // The directional shadow target is still bound here. Cloth must write before
    // the point-shadow pass switches to its cubemap framebuffer.
    cloth_.render_shadow(technique_frame, scene_);
    point_shadows_.render(technique_frame, scene_, resources_);
    render_rtt_feeds(technique_frame);

    hdr_target_.bind_for_write();
    gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    render_world_pass(technique_frame, true);
    render_to_texture_.render_displays(technique_frame, resources_, scene_);
    water_.capture_scene(frame.framebuffer_width, frame.framebuffer_height);
    water_.render(technique_frame, scene_, resources_);

    int bloom_result = 0;
    if (controls.bloom) bloom_result = bloom_.render(technique_frame, hdr_target_);
    post_.render(technique_frame, hdr_target_, bloom_, bloom_result);
    gpu_profiler_.end();
    gui_.render(frame, controls, capabilities_, scene_);
}

} // namespace epoch::render::detail
