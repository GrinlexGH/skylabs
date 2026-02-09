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

    [[nodiscard]] virtual Utils::Extent2D DrawableSize() const = 0;
};
