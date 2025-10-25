#pragma once
#include <span>

#include <SDL3/SDL_keyboard.h>

namespace SDL {
inline std::span<const bool> GetKeyboardState() {
    int keyboardStateSize = 0;
    SDL_GetKeyboardState(&keyboardStateSize);
    return { SDL_GetKeyboardState(nullptr), static_cast<std::size_t>(keyboardStateSize) };
}
}
