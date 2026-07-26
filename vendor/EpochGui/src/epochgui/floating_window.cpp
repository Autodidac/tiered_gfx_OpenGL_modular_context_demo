#include "gui/floating_window.hpp"
#include <algorithm>
#include <cmath>

namespace epochengine::gui_lib {
namespace {
float sane(float value, float fallback) noexcept { return std::isfinite(value) ? value : fallback; }
Vec2 clamp_size(Vec2 value, Vec2 minimum) noexcept {
    return {(std::max)(sane(value.x, minimum.x),(std::max)(1.0f,minimum.x)),
            (std::max)(sane(value.y, minimum.y),(std::max)(1.0f,minimum.y))};
}
Vec2 clamp_position(Vec2 p, Vec2 viewport) noexcept {
    if (viewport.x <= 0.0f || viewport.y <= 0.0f) return p;
    constexpr float visible = 48.0f;
    return {std::clamp(sane(p.x,0.0f),(std::min)(0.0f,viewport.x-visible),(std::max)(visible,viewport.x-visible)),
            std::clamp(sane(p.y,0.0f),0.0f,(std::max)(visible,viewport.y-visible))};
}
FloatingWindowLayout make_layout(const FloatingWindowState& s, const FloatingWindowOptions& o) noexcept {
    FloatingWindowLayout l{}; l.visible=s.open; l.window={s.position,s.size};
    const float title=(std::min)((std::max)(18.0f,o.title_bar_height),s.size.y); const float pad=(std::max)(0.0f,o.content_padding);
    l.title_bar={s.position,{s.size.x,title}};
    l.content={{s.position.x+pad,s.position.y+title+pad},{(std::max)(1.0f,s.size.x-2*pad),(std::max)(1.0f,s.size.y-title-2*pad)}};
    l.close_button={{s.position.x+(std::max)(0.0f,s.size.x-24.0f-pad),s.position.y+(std::max)(2.0f,(title-22.0f)*0.5f)},{24,22}};
    l.resize_handle={{s.position.x+(std::max)(0.0f,s.size.x-16.0f),s.position.y+(std::max)(0.0f,s.size.y-16.0f)},{16,16}};
    return l;
}
}
void normalize_floating_window(FloatingWindowState& s,const FloatingWindowOptions& o) noexcept {
    if(!s.initialized){s.position=o.default_position;s.size=o.default_size;s.initialized=true;}
    s.size=clamp_size(s.size,o.min_size);s.position=clamp_position(s.position,o.viewport_size);
}
FloatingWindowLayout update_floating_window(FloatingWindowState& s,const FloatingWindowOptions& o,const FloatingWindowInput& i) noexcept {
    normalize_floating_window(s,o); auto l=make_layout(s,o); if(!s.open)return l;
    l.hovered=contains(l.window,i.mouse_position);l.title_hovered=contains(l.title_bar,i.mouse_position);
    l.close_hovered=o.closable&&contains(l.close_button,i.mouse_position);l.resize_hovered=o.resizable&&contains(l.resize_handle,i.mouse_position);
    if(i.mouse_pressed&&l.hovered){l.focused=true;if(l.close_hovered)s.close_pressed=true;else if(l.resize_hovered){s.resizing=true;s.resize_origin_mouse=i.mouse_position;s.resize_origin_size=s.size;}else if(o.movable&&l.title_hovered){s.dragging=true;s.drag_offset={i.mouse_position.x-s.position.x,i.mouse_position.y-s.position.y};}}
    if(i.mouse_down&&s.dragging){s.position=clamp_position({i.mouse_position.x-s.drag_offset.x,i.mouse_position.y-s.drag_offset.y},o.viewport_size);l.moved=true;}
    if(i.mouse_down&&s.resizing){s.size=clamp_size({s.resize_origin_size.x+i.mouse_position.x-s.resize_origin_mouse.x,s.resize_origin_size.y+i.mouse_position.y-s.resize_origin_mouse.y},o.min_size);l.resized=true;}
    if(i.mouse_released){if(s.close_pressed&&l.close_hovered){l.close_requested=true;s.open=false;}s.dragging=false;s.resizing=false;s.close_pressed=false;}
    auto u=make_layout(s,o);u.hovered=contains(u.window,i.mouse_position);u.title_hovered=contains(u.title_bar,i.mouse_position);u.close_hovered=o.closable&&contains(u.close_button,i.mouse_position);u.resize_hovered=o.resizable&&contains(u.resize_handle,i.mouse_position);u.focused=l.focused;u.moved=l.moved;u.resized=l.resized;u.close_requested=l.close_requested;return u;
}
void FloatingWindowController::normalize(FloatingWindowState& s,const FloatingWindowOptions& o)const noexcept{normalize_floating_window(s,o);} 
FloatingWindowLayout FloatingWindowController::update(FloatingWindowState& s,const FloatingWindowOptions& o,const FloatingWindowInput& i)const noexcept{return update_floating_window(s,o,i);} 
const FloatingWindowController& floating_window_controller() noexcept { static const FloatingWindowController c{}; return c; }
}
