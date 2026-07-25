#include <skylabs/public/sdl/vulkan/surface_provider.hpp>
#include <skylabs/public/sdl/vulkan/vulkan.hpp>

namespace SDL::Vulkan {
std::span<const char* const> CSurfaceProvider::RequiredInstanceExtensions() const {
    return GetInstanceExtensions();
}

vk::SurfaceKHR CSurfaceProvider::CreateSurface(const vk::Instance instance) const {
    return Vulkan::CreateSurface(m_windowHandle, instance);
}
}
