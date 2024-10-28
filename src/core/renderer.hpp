#pragma once

#include "window.hpp"

class IRenderer {
public:
    // Initializing should be in constructor, destruction in descructor
    virtual ~IRenderer() = default;

    virtual void Draw() = 0;
};
