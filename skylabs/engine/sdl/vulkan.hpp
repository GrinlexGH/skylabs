#pragma once
#include <SDL3/SDL.h>
#include <vulkan/vulkan.hpp>

namespace sk::sdl::vulkan {
std::span<const char* const> GetInstanceExtensions();
vk::SurfaceKHR CreateSurface(SDL_Window* window, const vk::Instance& instance);
}
