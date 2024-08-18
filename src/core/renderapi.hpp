#pragma once

#include "window.hpp"

class IRenderApi {
public:
    IRenderApi() = default;
    IRenderApi(const IRenderApi&) = default;
    IRenderApi(IRenderApi&&) = default;
    IRenderApi& operator=(const IRenderApi&) = default;
    IRenderApi& operator=(IRenderApi&&) = default;
    virtual ~IRenderApi() = default;

    virtual void Init(IWindow*) = 0;
    virtual void Destroy() = 0;
};
