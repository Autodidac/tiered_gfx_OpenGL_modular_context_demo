#pragma once





#include <array>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#include "epoch/render/gl/shader_program.hpp"
#include "epoch/render/gl/texture.hpp"
#include "gui/control_layout.hpp"
#include "gui/floating_window.hpp"

#if defined(EPOCH_NO_MODULES)
#include "epoch/compat/epoch.core.math.hpp"
#else
import epoch.core.math;
#endif
#if defined(EPOCH_NO_MODULES)
#include "epoch/compat/epoch.context.input.hpp"
#else
import epoch.context.input;
#endif
#if defined(EPOCH_NO_MODULES)
#include "epoch/compat/epoch.context.frame.hpp"
#else
import epoch.context.frame;
#endif
#if defined(EPOCH_NO_MODULES)
#include "epoch/compat/epoch.render.types.hpp"
#else
import epoch.render.types;
#endif
#if defined(EPOCH_NO_MODULES)
#include "epoch/compat/epoch.render.scene.hpp"
#else
import epoch.render.scene;
#endif

namespace epoch::render { class ResourceSpine; }

namespace epoch::gui {
namespace gl = epoch::render::gl;

class GuiOverlay {
public:
    enum class EditorImportRequest : unsigned char {
        none,
        obj_model,
        albedo_texture,
        normal_texture,
        orm_texture
    };

    GuiOverlay(const std::filesystem::path& asset_root, render::ResourceSpine& resources);
    GuiOverlay(const GuiOverlay&) = delete;
    GuiOverlay& operator=(const GuiOverlay&) = delete;
    ~GuiOverlay();

    [[nodiscard]] bool update(context::InputState& input, const context::FrameContext& frame,
                              context::RuntimeControls& controls,
                              render::scene::SceneSpine& scene);
    void initialize_scene_editor(render::scene::SceneSpine& scene);
    [[nodiscard]] EditorImportRequest consume_import_request() noexcept;
    void apply_imported_mesh(render::scene::SceneSpine& scene, render::MeshHandle mesh, std::string_view display_name);
    void apply_imported_texture(render::scene::SceneSpine& scene, render::TextureHandle texture,
                                EditorImportRequest slot, std::string_view display_name, std::string_view relative_asset_path);
    void synchronize_scene_editor(render::scene::SceneSpine& scene);
    void render(const context::FrameContext& frame, const context::RuntimeControls& controls,
                const render::RenderCapabilities& capabilities,
                const render::scene::SceneSpine& scene);
    [[nodiscard]] bool captures_mouse() const noexcept { return captures_mouse_; }

private:
    struct Vertex {
        math::Vec2 position{};
        math::Vec2 uv{};
        math::Vec4 color{};
        float textured{};
    };

    void push_quad(float x, float y, float width, float height, math::Vec4 color,
                   math::Vec2 uv0 = {}, math::Vec2 uv1 = {1.0f, 1.0f}, float textured = 0.0f);
    void rectangle(epochengine::gui_lib::Rect rect, math::Vec4 color);
    void screen_rectangle(epochengine::gui_lib::Rect rect, math::Vec4 color);
    void text(float x, float y, std::string_view value, math::Vec4 color, float scale = 1.0f);
    void screen_text(float x, float y, std::string_view value, math::Vec4 color, float scale = 1.0f);
    void label_value(float x, float y, std::string_view label, float value);
    void build_panel(const context::FrameContext& frame, const context::RuntimeControls& controls,
                     const render::RenderCapabilities& capabilities);
    void build_scene_labels(const context::FrameContext& frame,
                            const context::RuntimeControls& controls,
                            const render::scene::SceneSpine& scene);
    void build_selection_marker(const context::FrameContext& frame,
                                const context::RuntimeControls& controls,
                                const render::scene::SceneSpine& scene);
    void build_reopen_button();
    void build_inspector(const render::scene::SceneSpine& scene);
    void update_inspector(context::InputState& input, const context::FrameContext& frame,
                          render::scene::SceneSpine& scene,
                          epochengine::gui_lib::Vec2 mouse,
                          const epochengine::gui_lib::FloatingWindowInput& window_input);
    void select_object_at(const context::InputState& input, const context::FrameContext& frame,
                          const context::RuntimeControls& controls, render::scene::SceneSpine& scene);
    void select_object(render::scene::SceneSpine& scene, std::size_t index);
    void deselect_all();
    void refresh_editor_selection(render::scene::SceneSpine& scene);
    void apply_editor_transform(render::scene::SceneSpine& scene);
    void duplicate_selected(render::scene::SceneSpine& scene);
    void sync_bound_entity(render::scene::SceneSpine& scene, std::size_t object_index);
    void sync_all_bound_entities(render::scene::SceneSpine& scene);
    void save_scene_overrides(const render::scene::SceneSpine& scene) const;
    void load_scene_overrides(render::scene::SceneSpine& scene);
    void save_foliage_masks(const render::scene::SceneSpine& scene) const;
    void load_foliage_masks(render::scene::SceneSpine& scene);
    void reset_selected(render::scene::SceneSpine& scene);
    [[nodiscard]] render::MaterialHandle ensure_unique_material(render::scene::SceneSpine& scene, std::size_t object_index);
    void reset_selected_texture_slot(render::scene::SceneSpine& scene);
    [[nodiscard]] std::vector<std::size_t> selected_indices(const render::scene::SceneSpine& scene) const;

    struct EditorSnapshot {
        render::scene::Transform transform{};
        math::Vec3 authored_position{};
        math::Vec3 authored_size{1.0f, 1.0f, 1.0f};
        math::Vec3 relative_scale{1.0f, 1.0f, 1.0f};
        render::MeshHandle mesh{};
        render::MaterialHandle material{};
        std::array<float, 4> properties{};
        bool visible{true};
        bool deleted{};
    };

    render::ResourceSpine& resources_;
    gl::ShaderProgram shader_;
    gl::Texture2D font_;
    GLuint vao_{};
    GLuint vbo_{};
    std::vector<Vertex> vertices_;
    epochengine::gui_lib::FloatingWindowState panel_{{14.0f, 34.0f}, {520.0f, 600.0f}};
    epochengine::gui_lib::FloatingWindowLayout panel_layout_{};
    std::array<epochengine::gui_lib::ButtonState, 64> tuning_scalar_button_states_{};
    std::array<epochengine::gui_lib::ButtonState, 48> inspector_scalar_button_states_{};
    std::array<epochengine::gui_lib::ButtonState, 24> inspector_button_states_{};
    epochengine::gui_lib::Rect reopen_button_{{18.0f, 12.0f}, {260.0f, 38.0f}};
    epochengine::gui_lib::FloatingWindowState inspector_{{700.0f, 34.0f}, {440.0f, 620.0f}};
    epochengine::gui_lib::FloatingWindowLayout inspector_layout_{};
    std::filesystem::path editor_save_path_{};
    std::filesystem::path grass_mask_path_{};
    std::vector<EditorSnapshot> editor_defaults_{};
    std::vector<std::array<std::uint8_t, render::scene::foliage_mask_texel_count>> foliage_mask_defaults_{};
    std::size_t selected_object_{static_cast<std::size_t>(-1)};
    std::vector<std::size_t> multi_selected_objects_{};
    std::size_t selected_model_option_{};
    std::size_t selected_material_option_{};
    EditorImportRequest selected_texture_slot_{EditorImportRequest::albedo_texture};
    math::Vec3 editor_position_{};
    math::Vec3 editor_rotation_degrees_{};
    math::Vec3 editor_size_{1.0f, 1.0f, 1.0f};
    math::Vec3 editor_scale_{1.0f, 1.0f, 1.0f};
    math::Vec3 previous_editor_position_{};
    math::Vec3 previous_editor_rotation_degrees_{};
    math::Vec3 previous_editor_size_{1.0f, 1.0f, 1.0f};
    math::Vec3 previous_editor_scale_{1.0f, 1.0f, 1.0f};
    bool editor_group_mode_{};
    bool manual_multiselect_{};
    EditorImportRequest import_request_{EditorImportRequest::none};
    std::size_t duplicate_serial_{};
    std::size_t authored_object_count_{};
    std::size_t authored_material_count_{};
    bool captures_mouse_{};
    float ui_scale_{1.4f};
    std::array<float, 5> tab_scroll_{};
    std::uint32_t selected_tab_{};
    std::uint32_t inspector_page_{};
    std::size_t active_numeric_edit_{static_cast<std::size_t>(-1)};
    std::string numeric_edit_buffer_{};
    std::size_t active_numeric_scrub_{static_cast<std::size_t>(-1)};
    float numeric_scrub_start_value_{};
    float numeric_scrub_start_mouse_x_{};
    bool numeric_scrub_dragged_{};
    float grass_brush_radius_{4.0f};
    bool grass_paint_add_{true};
};

} // namespace epoch::gui
