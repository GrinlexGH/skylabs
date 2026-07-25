#include <skylabs/public/sdl/sdl.hpp>

namespace SDL {
PUBLIC_CLASS Utils::Extent2D GetWindowSizeInPixels(SDL_Window* window) {
    int w, h;
    SDL_GetWindowSizeInPixels(window, &w, &h);
    return Utils::Extent2D {
        .m_width = static_cast<std::uint32_t>(w),
        .m_height = static_cast<std::uint32_t>(h)
    };
}

PUBLIC_CLASS std::span<const bool> GetKeyboardState() {
    int keyboardStateSize = 0;
    SDL_GetKeyboardState(&keyboardStateSize);
    return { SDL_GetKeyboardState(nullptr), static_cast<std::size_t>(keyboardStateSize) };
}
}
