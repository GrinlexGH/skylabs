#pragma once
#include <cstddef>

#include <SDL3/SDL_video.h>

#include "skylabs/base/vulkan/os_connector.hpp"

namespace sk::sdl::vulkan {
class SK_BASE_PUBLIC_CLASS OSConnector final : public sk::vulkan::IOSConnector {
public:
    explicit OSConnector(std::nullptr_t) { }
    explicit OSConnector(SDL_Window* window) : m_windowHandle(window) { }

    [[nodiscard]] PFN_vkGetInstanceProcAddr GetVkGetInstanceProcAddr() const override;
    [[nodiscard]] std::span<const char* const> RequiredInstanceExtensions() const override;
    [[nodiscard]] vk::SurfaceKHR CreateSurface(const vk::Instance& instance) const override;

private:
    SDL_Window* m_windowHandle = nullptr;
};
}
