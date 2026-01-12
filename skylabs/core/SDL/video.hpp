#pragma once
#include <skylabs/public/utils.hpp>

#include <SDL3/SDL_video.h>

namespace SDL {
inline Utils::CExtent2D GetWindowSizeInPixels(SDL_Window* window) {
    int w, h;
    SDL_GetWindowSizeInPixels(window, &w, &h);

    return Utils::CExtent2D {
        .width = static_cast<std::uint32_t>(w),
        .height = static_cast<std::uint32_t>(h)
    };
}
}
