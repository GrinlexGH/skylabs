#pragma once
#include "sk_base_export.h"
#include "skylabs/base/utils.hpp"

namespace sk {
class SK_BASE_PUBLIC_CLASS IWindow {
public:
    IWindow() = default;
    IWindow(const IWindow&) = delete;
    IWindow(IWindow&&) noexcept = default;
    IWindow& operator=(const IWindow&) = delete;
    IWindow& operator=(IWindow&&) noexcept = default;
    virtual ~IWindow() = default;

    [[nodiscard]] virtual utils::Extent2D DrawableSize() const = 0;
    [[nodiscard]] virtual bool IsMinimized() const = 0;
};
}
