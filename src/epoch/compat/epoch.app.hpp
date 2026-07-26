#pragma once

#include <filesystem>

#include "epoch/compat/epoch.context.spine.hpp"
#include "epoch/compat/epoch.render.spine.hpp"
namespace epoch::app {

class Application {
public:
    Application(context::NativeApplicationInstance instance, std::filesystem::path asset_root);
    int run();

private:
    context::ContextSpine context_;
    render::RenderSpine renderer_;
};

[[nodiscard]] std::filesystem::path resolve_asset_root();

} // namespace epoch::app
