#include <skylabs/public/sdl/mixer/context.hpp>

#include <SDL3_mixer/SDL_mixer.h>

#include "project_info.hpp"

namespace SDL::Mixer {
CContext::CContext() {
    if (!MIX_Init()) {
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
        MIX_Quit();
        m_initialized = false;
    }
}
}
