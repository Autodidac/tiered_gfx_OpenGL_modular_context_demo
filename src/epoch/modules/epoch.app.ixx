module;
#include <filesystem>

export module epoch.app;

export import epoch.context.spine;
export import epoch.render.spine;

export namespace epoch::app {

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
