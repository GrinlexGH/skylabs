#include <SDL3/SDL_vulkan.h>

#include "skylabs/base/sdl/vulkan/os_connector.hpp"
#include "skylabs/base/sdl/vulkan/vulkan.hpp"

namespace sk::sdl::vulkan {
PFN_vkGetInstanceProcAddr OSConnector::GetVkGetInstanceProcAddr() const {
    return reinterpret_cast<PFN_vkGetInstanceProcAddr>(SDL_Vulkan_GetVkGetInstanceProcAddr());
}

std::span<const char* const> OSConnector::RequiredInstanceExtensions() const {
    return GetInstanceExtensions();
}

vk::SurfaceKHR OSConnector::CreateSurface(const vk::Instance& instance) const {
    return vulkan::CreateSurface(m_windowHandle, instance);
}
}
