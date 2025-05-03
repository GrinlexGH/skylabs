#pragma once
#include <stdexcept>

class IRenderer
{
public:
    IRenderer() = default;
    IRenderer(const IRenderer&) = delete;
    IRenderer(IRenderer&&) = default;
    IRenderer& operator=(const IRenderer&) = delete;
    IRenderer& operator=(IRenderer&&) = default;
    virtual ~IRenderer() = default;

    virtual void Draw() = 0;
};
