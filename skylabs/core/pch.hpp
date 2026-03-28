#pragma once
#include <vulkan/vulkan_raii.hpp>
#include <vk_mem_alloc_raii.hpp>
#include <VkBootstrap.h>

#include <fmt/format.h>

#include <glm/glm.hpp>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#ifdef PLATFORM_WINDOWS
    #include <windows.h>
#endif

#include <unordered_map>
#include <unordered_set>
#include <vector>
