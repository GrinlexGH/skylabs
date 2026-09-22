#include <stdexcept>
#include <utility>

#include <SDL3_mixer/SDL_mixer.h>

#include "skylabs/base/sdl/mixer/context.hpp"

namespace sk::sdl::mixer {
Context::Context() {
    if (!MIX_Init()) {
        throw std::runtime_error(SDL_GetError());
    }

    m_initialized = true;
}

Context::Context(Context&& other) noexcept : m_initialized(std::exchange(other.m_initialized, 0)) { }

Context& Context::operator=(Context&& other) noexcept {
    if (this != &other) {
        Cleanup();
        m_initialized = std::exchange(other.m_initialized, 0);
    }
    return *this;
}

Context::~Context() { Cleanup(); }

void Context::Cleanup() {
    if (m_initialized) {
        MIX_Quit();
        m_initialized = false;
    }
}
}
