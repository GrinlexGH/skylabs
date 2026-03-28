#pragma once
#include <skylabs/public/utils.hpp>

class IWindow
{
public:
    IWindow() = default;
    IWindow(const IWindow&) = delete;
    IWindow(IWindow&&) noexcept = default;
    IWindow& operator=(const IWindow&) = delete;
    IWindow& operator=(IWindow&&) noexcept = default;
    virtual ~IWindow() = default;

    [[nodiscard]] virtual Utils::Extent2D DrawableSize() const = 0;

    // Vulkan
    [[nodiscard]] virtual bool IsQueueFamilySupportPresent(vk::Instance instance, vk::PhysicalDevice physicalDevice, std::uint32_t index) const = 0;
    [[nodiscard]] virtual vk::SurfaceKHR CreateSurface(vk::Instance instance) const = 0;
};
