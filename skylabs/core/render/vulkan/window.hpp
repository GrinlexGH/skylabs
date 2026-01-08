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

    [[nodiscard]] virtual auto GetRequiredInstanceExtensions() const -> std::span<const char* const> = 0;
    [[nodiscard]] virtual auto CreateSurface(const vk::Instance& instance) const -> vk::SurfaceKHR = 0;
    virtual auto DestroySurface(const vk::Instance& instance, vk::SurfaceKHR& surface) const -> void = 0;

    [[nodiscard]] virtual auto IsQueueFamilySupportPresent(
        const vk::Instance& instance,
        const vk::PhysicalDevice& physicalDevice,
        std::uint32_t index
    ) const -> bool = 0;
};
}
