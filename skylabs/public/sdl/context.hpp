#pragma once
#include <skylabs/public/pch.hpp>

namespace SDL {
class PUBLIC_CLASS CContext {
public:
    CContext() = delete;
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
