#include <SDL3/SDL_vulkan.h>
#include <fmt/format.h>

#include "skylabs/engine/sdl/vulkan.hpp"

namespace sk::sdl::vulkan {
std::span<const char* const> GetInstanceExtensions() {
    Uint32 extensionsSize = 0;
    SDL_Vulkan_GetInstanceExtensions(&extensionsSize);
    return { SDL_Vulkan_GetInstanceExtensions(nullptr), static_cast<std::size_t>(extensionsSize) };
}

vk::SurfaceKHR CreateSurface(SDL_Window* window, const vk::Instance& instance) {
    VkSurfaceKHR surface { };
    if (!SDL_Vulkan_CreateSurface(window, instance, nullptr, &surface)) {
        throw std::runtime_error(
            fmt::format("Failed to create vulkan surface via SDL: {}", SDL_GetError()));
    }
    return surface;
}
}
