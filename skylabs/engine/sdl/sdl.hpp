#pragma once
#include <span>

#include <SDL3/SDL.h>

#include "skylabs/engine/utils.hpp"

namespace sk::sdl {
std::span<const bool> GetKeyboardState();
utils::Extent2D GetWindowSizeInPixels(SDL_Window* window);
}
