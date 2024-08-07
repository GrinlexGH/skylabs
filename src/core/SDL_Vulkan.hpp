#pragma once

#include "SDL_vulkan.h"

namespace SDL {
    namespace Vulkan {
        inline bool GetRequiredInstanceExtensions(uint32_t* count, const char** names) {
            return SDL_Vulkan_GetInstanceExtensions(nullptr, count, names);
        }

        inline bool CreateSurface(SDL_Window* window,
                                  vk::Instance instance,
                                  vk::SurfaceKHR* surface) {
            return SDL_Vulkan_CreateSurface(window, instance, reinterpret_cast<VkSurfaceKHR*>(surface));
        }

        inline void GetDrawableSize(SDL_Window* window, int *w, int *h) {
            SDL_Vulkan_GetDrawableSize(window, w, h);
        }
    }
}