#pragma once
#include <skylabs/core/window.hpp>

#include <span>

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

    [[nodiscard]] virtual std::span<const char* const> GetRequiredInstanceExtensions() const = 0;
    [[nodiscard]] virtual vk::SurfaceKHR CreateSurface(const vk::Instance& instance) const = 0;
    virtual void DestroySurface(const vk::Instance& instance, vk::SurfaceKHR& surface) const = 0;

    [[nodiscard]] virtual bool IsQueueFamilySupportPresent(
        const vk::Instance& instance,
        const vk::PhysicalDevice& physicalDevice,
        std::uint32_t index
    ) const = 0;
};
}
