#pragma once
#include <SDL3/SDL.h>

namespace SDL {
class CGlobalContext
{
public:
    CGlobalContext();
    CGlobalContext(const CGlobalContext&) = delete;
    CGlobalContext(CGlobalContext&&) = delete;
    CGlobalContext& operator=(const CGlobalContext&) = delete;
    CGlobalContext& operator=(CGlobalContext&&) = delete;
    ~CGlobalContext();

private:
    static int m_refCount;
};

class CSubSystemContext
{
public:
    CSubSystemContext() = delete;
    explicit CSubSystemContext(SDL_InitFlags flags);
    CSubSystemContext(const CSubSystemContext&) = delete;
    CSubSystemContext(CSubSystemContext&&) = delete;
    CSubSystemContext& operator=(const CSubSystemContext&) = delete;
    CSubSystemContext& operator=(CSubSystemContext&&) = delete;
    ~CSubSystemContext();

private:
    SDL_InitFlags m_subSystems;
};
}
