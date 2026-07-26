module;
#include <filesystem>
#include <memory>

export module epoch.render.spine;

export import epoch.context.frame;
export import epoch.context.input;
export import epoch.render.types;
export import epoch.render.tier;
export import epoch.render.scene;
export import epoch.render.techniques.catalog;

export namespace epoch::render {

class RenderSpine {
public:
    explicit RenderSpine(std::filesystem::path asset_root);
    ~RenderSpine();

    RenderSpine(RenderSpine&&) noexcept;
    RenderSpine& operator=(RenderSpine&&) noexcept;
    RenderSpine(const RenderSpine&) = delete;
    RenderSpine& operator=(const RenderSpine&) = delete;

    void update(context::InputState& input,
                const context::FrameContext& frame,
                context::RuntimeControls& controls);
    void render(const context::FrameContext& frame,
                const context::RuntimeControls& controls);

    [[nodiscard]] const RenderCapabilities& capabilities() const noexcept;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace epoch::render
