#include "epoch/render/detail/render_spine_impl.hpp"
#include "epoch/render/detail/render_spine_bridge.hpp"

#include <filesystem>

extern "C" {

void* epoch_render_spine_create(const void* asset_root_path) {
    return new epoch::render::detail::RenderSpineImpl{
        *static_cast<const std::filesystem::path*>(asset_root_path)
    };
}

void epoch_render_spine_destroy(void* handle) noexcept {
    delete static_cast<epoch::render::detail::RenderSpineImpl*>(handle);
}

void epoch_render_spine_update(void* handle, void* input, const void* frame, void* controls) {
    static_cast<epoch::render::detail::RenderSpineImpl*>(handle)->update(
        *static_cast<epoch::context::InputState*>(input),
        *static_cast<const epoch::context::FrameContext*>(frame),
        *static_cast<epoch::context::RuntimeControls*>(controls));
}

void epoch_render_spine_render(void* handle, const void* frame, const void* controls) {
    static_cast<epoch::render::detail::RenderSpineImpl*>(handle)->render(
        *static_cast<const epoch::context::FrameContext*>(frame),
        *static_cast<const epoch::context::RuntimeControls*>(controls));
}

const void* epoch_render_spine_capabilities(const void* handle) noexcept {
    return &static_cast<const epoch::render::detail::RenderSpineImpl*>(handle)->capabilities();
}

} // extern "C"
