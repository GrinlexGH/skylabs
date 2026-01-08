#pragma once
#include <skylabs/public/utils.hpp>

class IWindow
{
public:
    IWindow() = default;
    IWindow(const IWindow&) = delete;
    IWindow(IWindow&&) noexcept = default;
    IWindow& operator=(const IWindow&) = delete;
    IWindow& operator=(IWindow&&) noexcept = default;
    virtual ~IWindow() = default;

    [[nodiscard]] virtual auto DrawableSize() const -> CExtent2D = 0;
};
