#include <skylabs/public/sdl/sdl.hpp>

#include <SDL3/SDL_vulkan.h>

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

namespace Vulkan {
PUBLIC_CLASS vk::SurfaceKHR CreateSurface(SDL_Window* window, const vk::Instance& instance) {
    VkSurfaceKHR surface {};
    if (!SDL_Vulkan_CreateSurface(window, instance, nullptr, &surface)) {
        throw std::runtime_error(fmt::format("Failed to create vulkan surface via SDL: {}", SDL_GetError()));
    }
    return surface;
}
}
}
