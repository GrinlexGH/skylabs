#pragma once

#include "../vulkan.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <vector>

namespace SDL
{

namespace Vulkan
{

inline std::vector<const char*> GetInstanceExtensions() {
    Uint32 extCount = 0;
    SDL_Vulkan_GetInstanceExtensions(&extCount);
    const char* const* extensions = SDL_Vulkan_GetInstanceExtensions(&extCount);
    return std::vector<const char*>(extensions, extensions + extCount);
}

inline bool GetPresentationSupport(vk::Instance instance, vk::PhysicalDevice physicalDevice, Uint32 queueFamilyIndex) {
    return SDL_Vulkan_GetPresentationSupport(instance, physicalDevice, queueFamilyIndex);
}

inline vk::SurfaceKHR CreateSurface(SDL_Window* window, vk::Instance instance) {
    VkSurfaceKHR surface {};
    if (!SDL_Vulkan_CreateSurface(window, instance, nullptr, &surface)) {
        throw std::runtime_error(std::format("Failed to create vulkan surface via SDL: {}!", SDL_GetError()));
    }
    return vk::SurfaceKHR(surface);
}

inline void DestroySurface(vk::Instance instance, vk::SurfaceKHR surface) {
    SDL_Vulkan_DestroySurface(instance, surface, nullptr);
}

}

}
