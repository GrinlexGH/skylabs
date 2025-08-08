#pragma once

class IWindow
{
public:
    IWindow() = default;
    IWindow(const IWindow&) = delete;
    IWindow(IWindow&&) noexcept = default;
    IWindow& operator=(const IWindow&) = delete;
    IWindow& operator=(IWindow&&) noexcept = default;
    virtual ~IWindow() = default;

    virtual void GetDrawableSize(int& w, int& h) const = 0;
};
