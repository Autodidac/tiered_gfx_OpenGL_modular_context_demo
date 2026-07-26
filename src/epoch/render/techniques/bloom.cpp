#include "epoch/render/techniques/bloom.hpp"

namespace epoch::render::techniques {

BloomTechnique::BloomTechnique(const std::filesystem::path& root)
    : blur_shader_{root / "bloom/fullscreen.vert", root / "bloom/blur.frag"} {
    gl::GenVertexArrays(1, &fullscreen_vao_);
}

BloomTechnique::~BloomTechnique() {
    if (fullscreen_vao_) gl::DeleteVertexArrays(1, &fullscreen_vao_);
}

void BloomTechnique::resize(int width, int height) {
    targets_.resize(width, height);
}

int BloomTechnique::render(const TechniqueContext& frame, const gl::HdrTarget& hdr) {
    gl::Disable(GL_DEPTH_TEST);
    gl::Disable(GL_CULL_FACE);
    gl::BindVertexArray(fullscreen_vao_);
    blur_shader_.bind();
    blur_shader_.set("uImage", 0);
    blur_shader_.set("uBrightImage", 1);
    blur_shader_.set("uThreshold", frame.controls.bloom_threshold);

    int last = 0;
    constexpr int blur_passes = 12;
    for (int pass = 0; pass < blur_passes; ++pass) {
        const int current = pass & 1;
        targets_.bind_for_write(current);
        blur_shader_.set("uHorizontal", current == 0 ? 1 : 0);
        blur_shader_.set("uPrefilter", pass == 0 ? 1 : 0);
        if (pass == 0) {
            hdr.bind_scene(0);
            hdr.bind_bright(1);
        } else {
            targets_.bind_texture(last, 0);
        }
        gl::DrawArrays(GL_TRIANGLES, 0, 3);
        last = current;
    }

    gl::BindVertexArray(0);
    gl::Enable(GL_CULL_FACE);
    gl::Enable(GL_DEPTH_TEST);
    return last;
}

} // namespace epoch::render::techniques
