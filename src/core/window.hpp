#pragma once

class IWindow
{
public:
    IWindow() = default;
    IWindow(const IWindow&) = delete;
    IWindow(IWindow&&) = default;
    IWindow& operator=(const IWindow&) = delete;
    IWindow& operator=(IWindow&&) = default;
    virtual ~IWindow() = default;

    virtual void GetDrawableSize(int* w, int* h) = 0;
};
