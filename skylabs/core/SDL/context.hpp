#pragma once
#include <SDL3/SDL_init.h>

namespace SDL {
class CContext
{
public:
    CContext() = delete;
    explicit CContext(SDL_InitFlags flags);
    CContext(const CContext&) = delete;
    CContext(CContext&&) = delete;
    CContext& operator=(const CContext&) = delete;
    CContext& operator=(CContext&&) = delete;
    ~CContext();

private:
    static int m_refCount;
};
}
