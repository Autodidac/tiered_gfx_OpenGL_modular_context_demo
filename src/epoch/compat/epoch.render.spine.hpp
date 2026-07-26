#pragma once

#include <filesystem>
#include <memory>

#include "epoch/compat/epoch.context.frame.hpp"
#include "epoch/compat/epoch.context.input.hpp"
#include "epoch/compat/epoch.render.types.hpp"
#include "epoch/compat/epoch.render.tier.hpp"
#include "epoch/compat/epoch.render.scene.hpp"
#include "epoch/compat/epoch.render.techniques.catalog.hpp"
namespace epoch::render {

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
