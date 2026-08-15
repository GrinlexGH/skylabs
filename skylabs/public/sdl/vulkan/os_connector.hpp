#pragma once
#include <skylabs/public/vulkan/os_connector.hpp>

namespace SDL::Vulkan {
class PUBLIC_CLASS COSConnector final : public ::Vulkan::IOSConnector
{
public:
    explicit COSConnector(std::nullptr_t) {}
    explicit COSConnector(SDL_Window* window) : m_windowHandle(window) {}

    [[nodiscard]] PFN_vkGetInstanceProcAddr GetVkGetInstanceProcAddr() const override;
    [[nodiscard]] std::span<const char* const> RequiredInstanceExtensions() const override;
    [[nodiscard]] vk::SurfaceKHR CreateSurface(const vk::Instance& instance) const override;

private:
    SDL_Window* m_windowHandle = nullptr;
};
}
