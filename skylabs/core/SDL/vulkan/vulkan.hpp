#pragma once
#include <skylabs/core/pch.hpp>

#include <span>

namespace SDL::Vulkan {
inline std::span<const char* const> GetInstanceExtensions() {
    Uint32 extCount = 0;
    SDL_Vulkan_GetInstanceExtensions(&extCount);
    return { SDL_Vulkan_GetInstanceExtensions(nullptr), extCount };
}

inline bool GetPresentationSupport(const vk::Instance& instance, const vk::PhysicalDevice& physicalDevice, const Uint32 queueFamilyIndex) {
    return SDL_Vulkan_GetPresentationSupport(instance, physicalDevice, queueFamilyIndex);
}

inline vk::SurfaceKHR CreateSurface(SDL_Window* window, const vk::Instance& instance) {
    VkSurfaceKHR surface {};
    if (!SDL_Vulkan_CreateSurface(window, instance, nullptr, &surface)) {
        throw std::runtime_error(fmt::format("Failed to create vulkan surface via SDL: {}!", SDL_GetError()));
    }
    return surface;
}

inline void DestroySurface(const vk::Instance& instance, const vk::SurfaceKHR& surface) {
    SDL_Vulkan_DestroySurface(instance, surface, nullptr);
}
}
