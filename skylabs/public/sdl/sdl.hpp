#pragma once
#include <skylabs/public/utils.hpp>

namespace SDL {
PUBLIC_CLASS std::span<const bool> GetKeyboardState();
PUBLIC_CLASS Utils::Extent2D GetWindowSizeInPixels(SDL_Window* window);
}
