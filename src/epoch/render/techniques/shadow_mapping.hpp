#pragma once


#include <filesystem>
#include "epoch/render/gl/frame_targets.hpp"
#include "epoch/render/gl/shader_program.hpp"
#include "epoch/render/resource_spine.hpp"

#if defined(EPOCH_NO_MODULES)
#include "epoch/compat/epoch.render.scene.hpp"
#else
import epoch.render.scene;
#endif
#if defined(EPOCH_NO_MODULES)
#include "epoch/compat/epoch.render.technique.context.hpp"
#else
import epoch.render.technique.context;
#endif

namespace epoch::render::techniques {

class ShadowMappingTechnique {
public:
    explicit ShadowMappingTechnique(const std::filesystem::path& shader_root);
    void render(const TechniqueContext& frame, const scene::SceneSpine& scene, const ResourceSpine& resources) const;
    void bind_depth(int unit) const noexcept { target_.bind_depth(unit); }

private:
    gl::ShaderProgram shader_;
    gl::ShadowTarget target_{2048};
};

} // namespace epoch::render::techniques
