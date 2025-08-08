#pragma once
#include "../../window.hpp"

#include <vector>
#include <cstdint>

#include <vulkan/vulkan.hpp>

namespace Vulkan {
class IWindow : public ::IWindow
{
public:
    IWindow() = default;
    IWindow(const IWindow&) = delete;
    IWindow(IWindow&&) noexcept = default;
    IWindow& operator=(const IWindow&) = delete;
    IWindow& operator=(IWindow&&) noexcept = default;
    ~IWindow() override = default;

    [[nodiscard]] virtual std::vector<const char*> GetRequiredInstanceExtensions() const = 0;
    [[nodiscard]] virtual bool CheckQueuePresentSupport(
        const vk::Instance& instance,
        const vk::PhysicalDevice& physicalDevice,
        std::uint32_t queueFamilyIndex
    ) const = 0;

    [[nodiscard]] virtual vk::SurfaceKHR CreateSurface(const vk::Instance& instance) const = 0;
    virtual void DestroySurface(const vk::Instance& instance, vk::SurfaceKHR& surface) const = 0;
};
}
