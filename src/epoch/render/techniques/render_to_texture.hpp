#pragma once


#include <array>
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

class RenderToTextureTechnique {
public:
    RenderToTextureTechnique(const std::filesystem::path& shader_root, MeshHandle screen_mesh);

    void resize(int width, int height);
    void begin_feed(scene::RttFeed feed) const;
    void render_displays(const TechniqueContext& frame, const ResourceSpine& resources,
                         const scene::SceneSpine& scene) const;

    [[nodiscard]] int feed_width() const noexcept { return feed_width_; }
    [[nodiscard]] int feed_height() const noexcept { return feed_height_; }

private:
    [[nodiscard]] static std::size_t feed_index(scene::RttFeed feed) noexcept;

    std::array<gl::HdrTarget, 4> targets_{};
    gl::ShaderProgram screen_;
    MeshHandle screen_mesh_{};
    int feed_width_{768};
    int feed_height_{432};
};

} // namespace epoch::render::techniques
