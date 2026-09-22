#include <stdexcept>
#include <utility>

#include <SDL3/SDL_error.h>
#include <SDL3_ttf/SDL_ttf.h>

#include "skylabs/base/sdl/ttf/context.hpp"

namespace sk::sdl::ttf {
Context::Context() {
    if (!TTF_Init()) {
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
        TTF_Quit();
        m_initialized = false;
    }
}
}
