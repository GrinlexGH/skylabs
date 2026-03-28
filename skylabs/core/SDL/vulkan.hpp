#pragma once
#include <skylabs/core/pch.hpp>

#include <span>

namespace SDL::Vulkan {
inline bool GetPresentationSupport(const vk::Instance& instance, const vk::PhysicalDevice& physicalDevice, const Uint32 queueFamilyIndex) {
    return SDL_Vulkan_GetPresentationSupport(instance, physicalDevice, queueFamilyIndex);
}

inline vk::SurfaceKHR CreateSurface(SDL_Window* window, const vk::Instance& instance) {
    VkSurfaceKHR surface {};
    if (!SDL_Vulkan_CreateSurface(window, instance, nullptr, &surface)) {
        throw std::runtime_error(fmt::format("Failed to create vulkan surface via SDL: {}", SDL_GetError()));
    }
    return surface;
}
}
