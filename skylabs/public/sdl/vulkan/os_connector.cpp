#include <skylabs/public/sdl/vulkan/os_connector.hpp>
#include <skylabs/public/sdl/vulkan/vulkan.hpp>

namespace SDL::Vulkan {
PFN_vkGetInstanceProcAddr COSConnector::GetVkGetInstanceProcAddr() const {
    return reinterpret_cast<PFN_vkGetInstanceProcAddr>(SDL_Vulkan_GetVkGetInstanceProcAddr());
}

std::span<const char* const> COSConnector::RequiredInstanceExtensions() const {
    return GetInstanceExtensions();
}

vk::SurfaceKHR COSConnector::CreateSurface(const vk::Instance& instance) const {
    return Vulkan::CreateSurface(m_windowHandle, instance);
}
}
