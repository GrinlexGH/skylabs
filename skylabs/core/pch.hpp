#pragma once
#include <vulkan/vulkan_raii.hpp>
#include <vk_mem_alloc_raii.hpp>
#include <VkBootstrap.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <fmt/format.h>
#include <glm/glm.hpp>
#include <entt/entt.hpp>

#include <boost/container/flat_set.hpp>
#include <boost/unordered/unordered_map.hpp>
#include <boost/unordered/unordered_set.hpp>

#ifdef PLATFORM_WINDOWS
    #include <windows.h>
#endif

#include <vector>
