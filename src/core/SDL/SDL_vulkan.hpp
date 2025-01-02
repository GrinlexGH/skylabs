#pragma once

#include "../render/vulkan/vulkan.hpp"

#include <SDL3/SDL_vulkan.h>

#include <vector>

namespace SDL::Vulkan
{
inline std::vector<const char*> GetInstanceExtensions() {
    Uint32 extCount = 0;
    SDL_Vulkan_GetInstanceExtensions(&extCount);
    const char* const* extensions = SDL_Vulkan_GetInstanceExtensions(&extCount);
    return std::vector(extensions, extensions + extCount);
}

inline bool GetPresentationSupport(
    const vk::Instance instance,
    const vk::PhysicalDevice physicalDevice,
    const Uint32 queueFamilyIndex
) {
    return SDL_Vulkan_GetPresentationSupport(instance, physicalDevice, queueFamilyIndex);
}

inline vk::SurfaceKHR CreateSurface(SDL_Window* window, const vk::Instance instance) {
    VkSurfaceKHR surface {};
    if (!SDL_Vulkan_CreateSurface(window, instance, nullptr, &surface)) {
        throw std::runtime_error(std::format("Failed to create vulkan surface via SDL: {}!", SDL_GetError()));
    }
    return surface;
}

inline void DestroySurface(const vk::Instance instance, const vk::SurfaceKHR surface) {
    SDL_Vulkan_DestroySurface(instance, surface, nullptr);
}
}
