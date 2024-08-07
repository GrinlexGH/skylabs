#pragma once

#include "window.hpp"

class IRenderApi {
public:
    virtual ~IRenderApi()   = default;

    virtual void Init(IWindow*)     = 0;
    virtual void Destroy()  = 0;
};
