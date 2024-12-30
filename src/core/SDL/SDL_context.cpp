#include "SDL_context.hpp"

#include <SDL3/SDL.h>

#include <stdexcept>
#include <format>

namespace SDL
{

CContext::CContext(const SDL_InitFlags flags)  {
    if (!SDL_Init(flags)) {
        throw std::runtime_error(std::format("Failed to initialize SDL: {}!", SDL_GetError()));
    }
}

CContext::~CContext() {
    SDL_Quit();
}

}
