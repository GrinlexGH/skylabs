#include <skylabs/public/sdl/vulkan/vulkan.hpp>

namespace SDL::Vulkan {
PUBLIC_CLASS std::span<const char* const> GetInstanceExtensions() {
    Uint32 extensionsSize = 0;
    SDL_Vulkan_GetInstanceExtensions(&extensionsSize);
    return { SDL_Vulkan_GetInstanceExtensions(nullptr), static_cast<std::size_t>(extensionsSize) };
}

PUBLIC_CLASS vk::SurfaceKHR CreateSurface(SDL_Window* window, const vk::Instance& instance) {
    VkSurfaceKHR surface {};
    if (!SDL_Vulkan_CreateSurface(window, instance, nullptr, &surface)) {
        throw std::runtime_error(fmt::format("Failed to create vulkan surface via SDL: {}", SDL_GetError()));
    }
    return surface;
}
}
