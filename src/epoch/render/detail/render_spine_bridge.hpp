#pragma once

extern "C" {
void* epoch_render_spine_create(const void* asset_root_path);
void epoch_render_spine_destroy(void* handle) noexcept;
void epoch_render_spine_update(void* handle, void* input, const void* frame, void* controls);
void epoch_render_spine_render(void* handle, const void* frame, const void* controls);
const void* epoch_render_spine_capabilities(const void* handle) noexcept;
}
