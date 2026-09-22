#pragma once
#include <cstddef>

#include <SDL3/SDL_init.h>

#include "sk_base_export.h"

namespace sk::sdl {
class SK_BASE_PUBLIC_CLASS Context {
public:
    Context() = delete;
    explicit Context(std::nullptr_t) { }
    explicit Context(SDL_InitFlags flags);
    Context(const Context&) = delete;
    Context(Context&& other) noexcept;
    Context& operator=(const Context&) = delete;
    Context& operator=(Context&& other) noexcept;
    ~Context();

private:
    void Cleanup();

    SDL_InitFlags m_flags = 0;
};
}
