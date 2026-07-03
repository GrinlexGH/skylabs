#include <skylabs/public/sdl/ttf/context.hpp>

namespace SDL::TTF {
CContext::CContext() {
    if (!TTF_Init()) {
        throw std::runtime_error(SDL_GetError());
    }

    m_initialized = true;
}

CContext::CContext(CContext&& other) noexcept :
    m_initialized(std::exchange(other.m_initialized, 0))
{}

CContext& CContext::operator=(CContext&& other) noexcept {
    if (this != &other) {
        Cleanup();
        m_initialized = std::exchange(other.m_initialized, 0);
    }
    return *this;
}

CContext::~CContext() {
    Cleanup();
}

void CContext::Cleanup() {
    if (m_initialized) {
        TTF_Quit();
        m_initialized = false;
    }
}
}
