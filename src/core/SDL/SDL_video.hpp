#pragma once

#include <SDL3/SDL.h>

namespace SDL
{

inline void GetWindowSizeInPixels(SDL_Window* window, int* w, int* h) {
    SDL_GetWindowSizeInPixels(window, w, h);
}

}
