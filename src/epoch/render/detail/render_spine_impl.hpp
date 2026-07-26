#pragma once



#include <filesystem>
#include "epoch/gui/gui_overlay.hpp"
#include "epoch/render/gl/frame_targets.hpp"
#include "epoch/render/resource_spine.hpp"
#include "epoch/render/techniques/billboard_foliage.hpp"
#include "epoch/render/techniques/bloom.hpp"
#include "epoch/render/techniques/cloth_simulation.hpp"
#include "epoch/render/techniques/instanced_props.hpp"
#include "epoch/render/techniques/particle_system.hpp"
#include "epoch/render/techniques/point_shadow_mapping.hpp"
#include "epoch/render/techniques/render_to_texture.hpp"
#include "epoch/render/techniques/pbr_forward.hpp"
#include "epoch/render/techniques/post_process.hpp"
#include "epoch/render/techniques/shadow_mapping.hpp"
#include "epoch/render/techniques/sky_environment.hpp"
#include "epoch/render/techniques/tessellation.hpp"
#include "epoch/render/techniques/water_simulation.hpp"
#include "epoch/render/techniques/gpu_profiler.hpp"

#if defined(EPOCH_NO_MODULES)
#include "epoch/compat/epoch.context.input.hpp"
#else
import epoch.context.input;
#endif
#if defined(EPOCH_NO_MODULES)
#include "epoch/compat/epoch.context.frame.hpp"
#else
import epoch.context.frame;
#endif
#if defined(EPOCH_NO_MODULES)
#include "epoch/compat/epoch.render.scene.hpp"
#else
import epoch.render.scene;
#endif

namespace epoch::render::detail {

class RenderSpineImpl {
public:
    explicit RenderSpineImpl(std::filesystem::path asset_root);
    RenderSpineImpl(const RenderSpineImpl&) = delete;
    RenderSpineImpl& operator=(const RenderSpineImpl&) = delete;

    void update(context::InputState& input, const context::FrameContext& frame, context::RuntimeControls& controls);
    void render(const context::FrameContext& frame, const context::RuntimeControls& controls);
    [[nodiscard]] const RenderCapabilities& capabilities() const noexcept { return capabilities_; }

private:
    [[nodiscard]] techniques::TechniqueContext make_technique_context(const context::FrameContext& frame,
                                                                       const context::RuntimeControls& controls) const;
    [[nodiscard]] techniques::TechniqueContext make_feed_context(
        const techniques::TechniqueContext& base,
        math::Vec3 position,
        math::Vec3 forward,
        math::Vec3 up,
        float vertical_fov_degrees) const;
    void render_world_pass(const techniques::TechniqueContext& frame, bool include_transparents) const;
    void render_rtt_feeds(const techniques::TechniqueContext& base);
    void detect_capabilities();

    ResourceSpine resources_;
    scene::SceneSpine scene_{};
    MeshHandle sky_mesh_{};
    MeshHandle billboard_mesh_{};
    MeshHandle instanced_crate_mesh_{};
    MeshHandle terrain_patch_mesh_{};
    RenderCapabilities capabilities_{};
    gl::HdrTarget hdr_target_{};
    techniques::ShadowMappingTechnique shadows_;
    techniques::PointShadowMappingTechnique point_shadows_;
    techniques::SkyEnvironmentTechnique sky_;
    techniques::PbrForwardTechnique pbr_;
    techniques::BillboardFoliageTechnique billboards_;
    techniques::ParticleSystemTechnique particles_;
    techniques::InstancedPropsTechnique instanced_props_;
    techniques::RenderToTextureTechnique render_to_texture_;
    techniques::TessellationTechnique tessellation_;
    techniques::ClothSimulationTechnique cloth_;
    techniques::WaterSimulationTechnique water_;
    techniques::GpuProfiler gpu_profiler_{};
    techniques::BloomTechnique bloom_;
    techniques::PostProcessTechnique post_;
    gui::GuiOverlay gui_;
};

} // namespace epoch::render::detail
