#pragma once
#include <SDL3/SDL_init.h>
#include <cstddef>

namespace SDL {
class CContext
{
public:
    CContext() = delete;
    explicit CContext(std::nullptr_t) noexcept {}
    explicit CContext(SDL_InitFlags flags);
    CContext(const CContext&) = delete;
    CContext(CContext&&) = default;
    CContext& operator=(const CContext&) = delete;
    CContext& operator=(CContext&&) = default;
    ~CContext();

private:
    static int m_refCount;
};
}
