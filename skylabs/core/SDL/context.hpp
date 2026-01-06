#pragma once
#include <SDL3/SDL_init.h>
#include <cstddef>

namespace SDL {
class CContext {
public:
    CContext() = delete;
    explicit CContext(std::nullptr_t) noexcept {}
    explicit CContext(SDL_InitFlags flags);
    CContext(const CContext&) = delete;
    CContext(CContext&& other) noexcept;
    CContext& operator=(const CContext&) = delete;
    CContext& operator=(CContext&& other) noexcept;
    ~CContext();

private:
    void Cleanup();

    SDL_InitFlags m_flags = 0;
};
}
