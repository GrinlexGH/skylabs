#include <SDL3/SDL_vulkan.h>

#include "skylabs/engine/sdl/vulkan.hpp"
#include "skylabs/engine/sdl/vulkan_adapter.hpp"

namespace sk::sdl::vulkan {
PFN_vkGetInstanceProcAddr OSAdapter::GetVkGetInstanceProcAddr() const {
    return reinterpret_cast<PFN_vkGetInstanceProcAddr>(SDL_Vulkan_GetVkGetInstanceProcAddr());
}

std::span<const char* const> OSAdapter::RequiredInstanceExtensions() const {
    return GetInstanceExtensions();
}

vk::SurfaceKHR OSAdapter::CreateSurface(const vk::Instance& instance) const {
    return vulkan::CreateSurface(m_windowHandle, instance);
}
}
