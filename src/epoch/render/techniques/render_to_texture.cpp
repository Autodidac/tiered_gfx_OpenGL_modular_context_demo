#include "epoch/render/techniques/render_to_texture.hpp"

namespace epoch::render::techniques {

RenderToTextureTechnique::RenderToTextureTechnique(const std::filesystem::path& root, MeshHandle mesh)
    : screen_{root / "rtt/screen.vert", root / "rtt/screen.frag"},
      screen_mesh_{mesh} {
    resize(feed_width_, feed_height_);
}

std::size_t RenderToTextureTechnique::feed_index(scene::RttFeed feed) noexcept {
    switch (feed) {
    case scene::RttFeed::security_camera: return 0u;
    case scene::RttFeed::overhead_camera: return 1u;
    case scene::RttFeed::ceiling_camera: return 2u;
    case scene::RttFeed::planar_mirror: return 3u;
    }
    return 0u;
}

void RenderToTextureTechnique::resize(int width, int height) {
    feed_width_ = width > 0 ? width : 768;
    feed_height_ = height > 0 ? height : 432;
    for (auto& target : targets_) target.resize(feed_width_, feed_height_);
}

void RenderToTextureTechnique::begin_feed(scene::RttFeed feed) const {
    targets_[feed_index(feed)].bind_for_write();
    gl::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void RenderToTextureTechnique::render_displays(
    const TechniqueContext& frame, const ResourceSpine& resources,
    const scene::SceneSpine& scene) const {
    if (!frame.controls.render_to_texture) return;

    screen_.bind();
    screen_.set("uViewProjection", frame.view_projection);
    screen_.set("uScreen", 0);
    screen_.set("uTime", frame.elapsed_seconds);
    for (const auto& display : scene.rtt_displays) {
        if (!display.visible) continue;
        if (display.feed == scene::RttFeed::planar_mirror && !frame.controls.mirror_rtt) continue;

        // RTT surfaces are two-sided. The mirror's opaque rear housing, rather than
        // rasterizer culling, determines which physical side is visible.
        gl::Disable(GL_CULL_FACE);

        screen_.set("uModel", display.transform.matrix());
        screen_.set("uFeedMode", static_cast<int>(display.feed));
        targets_[feed_index(display.feed)].bind_scene(0);
        resources.mesh(screen_mesh_).draw();
    }

    gl::Enable(GL_CULL_FACE);
    gl::CullFace(GL_BACK);
    gl::FrontFace(GL_CCW);
}

} // namespace epoch::render::techniques
