#include "skylabs/engine/sdl/sdl.hpp"

namespace sk::sdl {
std::span<const bool> GetKeyboardState() {
    int keyboardStateSize = 0;
    SDL_GetKeyboardState(&keyboardStateSize);
    return { SDL_GetKeyboardState(nullptr), static_cast<std::size_t>(keyboardStateSize) };
}

utils::Extent2D GetWindowSizeInPixels(SDL_Window* window) {
    int w, h;
    SDL_GetWindowSizeInPixels(window, &w, &h);
    return utils::Extent2D { .width = static_cast<std::uint32_t>(w),
                             .height = static_cast<std::uint32_t>(h) };
}
}
