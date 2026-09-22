#pragma once
#include <SDL3/SDL_video.h>
#include <vulkan/vulkan.hpp>

#include "sk_base_export.h"

namespace sk::sdl::vulkan {
SK_BASE_PUBLIC_CLASS std::span<const char* const> GetInstanceExtensions();
SK_BASE_PUBLIC_CLASS vk::SurfaceKHR CreateSurface(SDL_Window* window, const vk::Instance& instance);
}
