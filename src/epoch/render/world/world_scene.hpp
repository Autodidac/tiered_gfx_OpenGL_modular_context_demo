#pragma once

#include "epoch/render/resource_spine.hpp"

#if defined(EPOCH_NO_MODULES)
#include "epoch/compat/epoch.render.scene.hpp"
#else
import epoch.render.scene;
#endif

namespace epoch::render::world {

class WorldSceneBuilder {
public:
    static void build(ResourceSpine& resources, scene::SceneSpine& scene);
    static void apply_authored_defaults(ResourceSpine& resources, scene::SceneSpine& scene);
    [[nodiscard]] static bool write_authored_default_config(
        const std::filesystem::path& path, bool overwrite = false);
};

} // namespace epoch::render::world
