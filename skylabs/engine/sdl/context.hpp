#pragma once
#include <cstddef>

#include <SDL3/SDL.h>

namespace sk::sdl {
class Context {
public:
    Context() = delete;
    explicit Context(std::nullptr_t) { }
    explicit Context(SDL_InitFlags flags);
    Context(const Context&) = delete;
    Context(Context&&) = delete;
    Context& operator=(const Context&) = delete;
    Context& operator=(Context&&) = delete;
    ~Context();

private:
    void Cleanup();

    SDL_InitFlags m_flags = 0;
};
}
