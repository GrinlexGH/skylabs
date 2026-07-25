#pragma once
#include <skylabs/public/vulkan/surface_provider.hpp>

namespace SDL::Vulkan {
class PUBLIC_CLASS CSurfaceProvider : public ::Vulkan::ISurfaceProvider
{
public:
    explicit CSurfaceProvider(std::nullptr_t) {}
    explicit CSurfaceProvider(SDL_Window* window) : m_windowHandle(window) {}

    [[nodiscard]] std::span<const char* const> RequiredInstanceExtensions() const override;
    [[nodiscard]] vk::SurfaceKHR CreateSurface(vk::Instance instance) const override;

private:
    SDL_Window* m_windowHandle = nullptr;
};
}
