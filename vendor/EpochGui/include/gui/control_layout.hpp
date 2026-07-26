#pragma once
#include "floating_window.hpp"
#include <cstdint>

namespace epochengine::gui_lib {
inline constexpr std::uint32_t invalid_selectable_row_index = 0xffffffffU;
struct ButtonState { bool held{}; };
struct ButtonInput { Vec2 mouse_position{}; bool mouse_down{}; bool mouse_pressed{}; bool mouse_released{}; };
struct ButtonLayout { Rect bounds{}; bool hovered{}; bool held{}; bool activated{}; };
[[nodiscard]] ButtonLayout update_button(ButtonState& state, Rect bounds, const ButtonInput& input) noexcept;

struct ToggleLayout { Rect bounds{}; Rect track{}; Rect thumb{}; bool hovered{}; bool value{}; bool changed{}; };
[[nodiscard]] ToggleLayout update_toggle(bool& value, Rect bounds, const ButtonInput& input) noexcept;

struct SliderState { bool dragging{}; };
struct SliderLayout { Rect bounds{}; Rect track{}; Rect fill{}; Rect thumb{}; float fraction{}; bool hovered{}; bool changed{}; };
[[nodiscard]] SliderLayout update_slider(SliderState& state, float& value, float minimum, float maximum, Rect bounds,
                                         const ButtonInput& input) noexcept;

struct SegmentedControlLayoutOptions { Vec2 position{}; const float* item_widths{}; std::uint32_t item_count{}; float height{26.0f}; float gap{2.0f}; };
struct SegmentedControlLayout { Rect bounds{}; std::uint32_t item_count{}; float height{}; float gap{}; bool valid{}; };
[[nodiscard]] SegmentedControlLayout make_segmented_control_layout(const SegmentedControlLayoutOptions& options) noexcept;
[[nodiscard]] Rect segmented_control_item_layout(const SegmentedControlLayoutOptions& options, std::uint32_t index) noexcept;
[[nodiscard]] std::uint32_t segmented_control_item_at(const SegmentedControlLayoutOptions& options, Vec2 point) noexcept;
}
