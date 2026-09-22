#pragma once
#include <span>

#include <SDL3/SDL_video.h>

#include "sk_base_export.h"
#include "skylabs/base/utils.hpp"

namespace sk::sdl {
SK_BASE_PUBLIC_CLASS std::span<const bool> GetKeyboardState();
SK_BASE_PUBLIC_CLASS utils::Extent2D GetWindowSizeInPixels(SDL_Window* window);
}
