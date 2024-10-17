#pragma once

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>

#include "SDL3/SDL_vulkan.h"

namespace SDL {
    namespace Vulkan {
        inline const char* const* GetRequiredInstanceExtensions(uint32_t* count) {
            return SDL_Vulkan_GetInstanceExtensions(count);
        }

        inline bool CreateSurface(SDL_Window* window, vk::Instance instance, vk::SurfaceKHR* surface) {
            return SDL_Vulkan_CreateSurface(window, instance, nullptr, reinterpret_cast<VkSurfaceKHR*>(surface));
        }

        inline bool GetDrawableSize(SDL_Window* window, int* w, int* h) {
            return SDL_GetWindowSizeInPixels(window, w, h);
        }
    }
}