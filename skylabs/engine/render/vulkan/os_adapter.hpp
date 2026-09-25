#pragma once
#include <span>

#include <vulkan/vulkan.hpp>

namespace sk::render::vulkan {
class IOSAdapter {
public:
    virtual ~IOSAdapter() = default;

    [[nodiscard]] virtual PFN_vkGetInstanceProcAddr GetVkGetInstanceProcAddr() const = 0;
    [[nodiscard]] virtual std::span<const char* const> RequiredInstanceExtensions() const = 0;
    [[nodiscard]] virtual vk::SurfaceKHR CreateSurface(const vk::Instance& instance) const = 0;
};
}
