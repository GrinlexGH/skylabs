#pragma once

#include "window.hpp"

class IRenderer {
public:
    virtual ~IRenderer() = default;

    virtual void Initialize(IWindow*) = 0;
    virtual void Destroy() = 0;
};
