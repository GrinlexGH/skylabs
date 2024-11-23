#pragma once

#include <SDL3/SDL.h>

namespace SDL
{

class CContext
{
public:
    CContext() = delete;
    explicit CContext(SDL_InitFlags flags = 0);
    CContext(const CContext&) = delete;
    CContext(CContext&&) = delete;
    CContext& operator=(const CContext&) = delete;
    CContext& operator=(CContext&&) = delete;
    ~CContext();
};

}
