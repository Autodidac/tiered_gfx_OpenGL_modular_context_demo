#pragma once

#include <memory>

#include "epoch/compat/epoch.context.frame.hpp"
#include "epoch/compat/epoch.context.input.hpp"
namespace epoch::context {

using NativeApplicationInstance = void*;
using NativeWindowHandle = void*;

class ContextSpine {
public:
    explicit ContextSpine(NativeApplicationInstance instance);
    ~ContextSpine();

    ContextSpine(ContextSpine&&) noexcept;
    ContextSpine& operator=(ContextSpine&&) noexcept;
    ContextSpine(const ContextSpine&) = delete;
    ContextSpine& operator=(const ContextSpine&) = delete;

    [[nodiscard]] bool begin_frame(FrameContext& frame);
    void end_frame() noexcept;
    void apply_runtime_controls();

    [[nodiscard]] InputState& input() noexcept;
    [[nodiscard]] const RuntimeControls& controls() const noexcept;
    [[nodiscard]] RuntimeControls& controls() noexcept;
    [[nodiscard]] NativeWindowHandle native_window() const noexcept;
    void update_title(float fps) const noexcept;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace epoch::context
