#pragma once
#include <VkBootstrap.h>
#include <vulkan/vulkan_raii.hpp>
#include <vk_mem_alloc_raii.hpp>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <entt/entt.hpp>

#include <frozen/unordered_map.h>
#include <frozen/map.h>

#include <boost/range/irange.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/container/flat_set.hpp>
#include <boost/unordered/unordered_map.hpp>
#include <boost/unordered/unordered_set.hpp>

#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtc/matrix_transform.hpp>

#ifdef PLATFORM_WINDOWS
    #include <windows.h>
#endif

#include <cassert>
#include <deque>
