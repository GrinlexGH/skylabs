#pragma once
#include <cassert>
#include <deque>

#include <VkBootstrap.h>
#include <vk_mem_alloc_raii.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <entt/entt.hpp>

#include <frozen/map.h>
#include <frozen/unordered_map.h>

#include <boost/container/flat_map.hpp>
#include <boost/container/flat_set.hpp>
#include <boost/range/irange.hpp>
#include <boost/unordered/unordered_map.hpp>
#include <boost/unordered/unordered_set.hpp>

#include <glm/ext.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#ifdef PLATFORM_WINDOWS
#include <windows.h>
#endif
