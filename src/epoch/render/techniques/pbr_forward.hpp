#pragma once


#include <filesystem>
#include "epoch/render/gl/shader_program.hpp"
#include "epoch/render/resource_spine.hpp"
#include "epoch/render/techniques/shadow_mapping.hpp"
#include "epoch/render/techniques/point_shadow_mapping.hpp"

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

class PbrForwardTechnique {
public:
    PbrForwardTechnique(const std::filesystem::path& shader_root, ResourceSpine& resources);
    void render(const TechniqueContext& frame, const scene::SceneSpine& scene, const ResourceSpine& resources,
                const ShadowMappingTechnique& shadows,
                const PointShadowMappingTechnique& point_shadows) const;

private:
    gl::ShaderProgram shader_;
    TextureHandle projector_gobo_{};
};

} // namespace epoch::render::techniques
