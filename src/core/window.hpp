#pragma once

class IWindow
{
public:
    IWindow() = default;
    IWindow(const IWindow&) = default;
    IWindow(IWindow&&) = default;
    IWindow& operator=(const IWindow&) = default;
    IWindow& operator=(IWindow&&) = default;
    virtual ~IWindow() = default;
};
