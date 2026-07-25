#pragma once
#include <skylabs/public/pch.hpp>

namespace SDL::Vulkan {
PUBLIC_CLASS std::span<const char* const> GetInstanceExtensions();
PUBLIC_CLASS vk::SurfaceKHR CreateSurface(SDL_Window* window, const vk::Instance& instance);
}
