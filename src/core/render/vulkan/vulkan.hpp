#pragma once

#ifdef PLATFORM_WINDOWS
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>
