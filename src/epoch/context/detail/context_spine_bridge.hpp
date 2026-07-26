#pragma once

extern "C" {
void* epoch_context_spine_create(void* native_instance);
void epoch_context_spine_destroy(void* handle) noexcept;
bool epoch_context_spine_begin_frame(void* handle, void* frame);
void epoch_context_spine_end_frame(void* handle) noexcept;
void epoch_context_spine_apply_controls(void* handle);
void* epoch_context_spine_input(void* handle) noexcept;
const void* epoch_context_spine_controls_const(const void* handle) noexcept;
void* epoch_context_spine_controls(void* handle) noexcept;
void* epoch_context_spine_native_window(const void* handle) noexcept;
void epoch_context_spine_update_title(const void* handle, float fps) noexcept;
}
