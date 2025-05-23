#pragma once

class IWindow
{
public:
    IWindow() = default;
    IWindow(const IWindow&) = delete;
    IWindow(IWindow&&) = delete;
    IWindow& operator=(const IWindow&) = delete;
    IWindow& operator=(IWindow&&) = delete;
    virtual ~IWindow() = default;

    virtual void GetDrawableSize(int* w, int* h) const = 0;
};
