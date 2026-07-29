#pragma once
#include <skylabs/public/pch.hpp>

namespace Vulkan {
class PUBLIC_CLASS IOSConnector
{
public:
    virtual ~IOSConnector() = default;

    [[nodiscard]] virtual PFN_vkGetInstanceProcAddr GetVkGetInstanceProcAddr() const = 0;
    [[nodiscard]] virtual std::span<const char* const> RequiredInstanceExtensions() const = 0;
    [[nodiscard]] virtual vk::SurfaceKHR CreateSurface(vk::Instance instance) const = 0;
};
}
