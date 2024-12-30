#pragma once

#include "../window.hpp"

class IRenderer {
public:
    IRenderer() = default;
    IRenderer(const IRenderer&) = delete;
    IRenderer(IRenderer&& other) = default;
    IRenderer& operator=(const IRenderer&) = delete;
    IRenderer& operator=(IRenderer&& other) = default;
    virtual ~IRenderer() = default;

    virtual bool Initialize(IWindow* window) = 0;
    virtual void Draw() = 0;
};
