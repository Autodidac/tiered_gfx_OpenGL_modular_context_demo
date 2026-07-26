#include "epoch/render/techniques/post_process.hpp"

namespace epoch::render::techniques {

PostProcessTechnique::PostProcessTechnique(const std::filesystem::path& root)
    : shader_{root / "post/fullscreen.vert", root / "post/post.frag"} {
    gl::GenVertexArrays(1, &fullscreen_vao_);
}

PostProcessTechnique::~PostProcessTechnique() {
    if (fullscreen_vao_) gl::DeleteVertexArrays(1, &fullscreen_vao_);
}

void PostProcessTechnique::render(
    const TechniqueContext& frame, const gl::HdrTarget& hdr,
    const BloomTechnique& bloom, int bloom_index) const {
    gl::BindFramebuffer(gl::framebuffer, 0);
    gl::Viewport(0, 0, frame.width, frame.height);
    gl::Disable(GL_DEPTH_TEST);
    gl::Disable(GL_CULL_FACE);
    gl::Clear(GL_COLOR_BUFFER_BIT);
    gl::BindVertexArray(fullscreen_vao_);

    shader_.bind();
    shader_.set("uScene", 0);
    shader_.set("uBloom", 1);
    shader_.set("uDepth", 2);
    shader_.set("uInvResolution", math::Vec2{1.0f / frame.width, 1.0f / frame.height});
    shader_.set("uProjection", frame.projection);
    shader_.set("uExposure", frame.controls.exposure);
    shader_.set("uGamma", frame.controls.gamma);
    shader_.set("uBloomStrength", frame.controls.bloom_strength);
    shader_.set("uBloomEnabled", frame.controls.bloom ? 1 : 0);
    shader_.set("uFxaaEnabled", frame.controls.fxaa ? 1 : 0);
    shader_.set("uSsaoEnabled", (frame.controls.ssao && !frame.controls.tier0_mobile_profile) ? 1 : 0);
    shader_.set("uSsaoStrength", frame.controls.ssao_strength);
    shader_.set("uSsaoRadius", frame.controls.ssao_radius);
    shader_.set("uNearPlane", frame.camera_near);
    shader_.set("uFarPlane", frame.camera_far);

    hdr.bind_scene(0);
    bloom.bind_result(bloom_index, 1);
    hdr.bind_depth(2);
    gl::DrawArrays(GL_TRIANGLES, 0, 3);

    gl::BindVertexArray(0);
    gl::Enable(GL_CULL_FACE);
    gl::Enable(GL_DEPTH_TEST);
}

} // namespace epoch::render::techniques
