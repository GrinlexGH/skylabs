#pragma once
#include <skylabs/public/vulkan/os_connector.hpp>

namespace SDL::Vulkan {
class PUBLIC_CLASS CSurfaceProvider final : public ::Vulkan::IOSConnector
{
public:
    explicit CSurfaceProvider(std::nullptr_t) {}
    explicit CSurfaceProvider(SDL_Window* window) : m_windowHandle(window) {}

    [[nodiscard]] PFN_vkGetInstanceProcAddr GetVkGetInstanceProcAddr() const override;
    [[nodiscard]] std::span<const char* const> RequiredInstanceExtensions() const override;
    [[nodiscard]] vk::SurfaceKHR CreateSurface(vk::Instance instance) const override;

private:
    SDL_Window* m_windowHandle = nullptr;
};
}
