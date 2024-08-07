#pragma once

class IWindow {
public:
    IWindow()                           = default;
    IWindow(const IWindow&)             = delete;
    IWindow(IWindow&&)                  = default;
    IWindow& operator=(const IWindow&)  = delete;
    IWindow& operator=(IWindow&&)       = default;
    virtual ~IWindow()                  = default;

    // \param title     UTF-8 encoded window title
    // \param x         X position of window or value provided with your window library
    // \param y         Y position of window or value provided with your window library
    // \param w         width of window
    // \param h         height of window
    // \param flags     flags which provided with your window library e.g. SDL, GLFW, WINAPI
    virtual void  Create(const char* title, int x, int y, int w, int h, unsigned int flags) = 0;
    virtual void  Close()           = 0;
    virtual void* GetHandle() const = 0;
};
