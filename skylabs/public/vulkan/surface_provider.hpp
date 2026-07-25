#pragma once
#include <skylabs/public/pch.hpp>

namespace Vulkan {
class ISurfaceProvider
{
public:
    virtual ~ISurfaceProvider() = default;

    [[nodiscard]] virtual std::span<const char* const> RequiredInstanceExtensions() const = 0;
    [[nodiscard]] virtual vk::SurfaceKHR CreateSurface(vk::Instance instance) const = 0;
};
}
