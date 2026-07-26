#pragma once

#include <filesystem>
#include "epoch/render/gl/frame_targets.hpp"
#include "epoch/render/gl/shader_program.hpp"

#if defined(EPOCH_NO_MODULES)
#include "epoch/compat/epoch.render.technique.context.hpp"
#else
import epoch.render.technique.context;
#endif

namespace epoch::render::techniques {

class BloomTechnique {
public:
    explicit BloomTechnique(const std::filesystem::path& shader_root);
    BloomTechnique(const BloomTechnique&) = delete;
    BloomTechnique& operator=(const BloomTechnique&) = delete;
    ~BloomTechnique();

    void resize(int width, int height);
    [[nodiscard]] int render(const TechniqueContext& frame, const gl::HdrTarget& hdr);
    void bind_result(int index, int unit) const noexcept { targets_.bind_texture(index, unit); }

private:
    gl::ShaderProgram blur_shader_;
    gl::BlurTargets targets_;
    GLuint fullscreen_vao_{};
};

} // namespace epoch::render::techniques
