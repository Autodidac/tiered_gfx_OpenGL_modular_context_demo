#pragma once

#include <filesystem>
#include "epoch/render/gl/frame_targets.hpp"
#include "epoch/render/gl/shader_program.hpp"
#include "epoch/render/techniques/bloom.hpp"

#if defined(EPOCH_NO_MODULES)
#include "epoch/compat/epoch.render.technique.context.hpp"
#else
import epoch.render.technique.context;
#endif

namespace epoch::render::techniques {

class PostProcessTechnique {
public:
    explicit PostProcessTechnique(const std::filesystem::path& shader_root);
    PostProcessTechnique(const PostProcessTechnique&) = delete;
    PostProcessTechnique& operator=(const PostProcessTechnique&) = delete;
    ~PostProcessTechnique();

    void render(const TechniqueContext& frame, const gl::HdrTarget& hdr,
                const BloomTechnique& bloom, int bloom_index) const;

private:
    gl::ShaderProgram shader_;
    GLuint fullscreen_vao_{};
};

} // namespace epoch::render::techniques
