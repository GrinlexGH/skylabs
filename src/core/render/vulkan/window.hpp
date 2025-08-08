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

    [[nodiscard]] virtual auto GetRequiredInstanceExtensions() const -> std::vector<const char*> = 0;

    [[nodiscard]] virtual auto CreateSurface(const vk::Instance& instance) const -> vk::SurfaceKHR = 0;
    virtual auto DestroySurface(const vk::Instance& instance, vk::SurfaceKHR& surface) const -> void = 0;

    [[nodiscard]] virtual auto IsQueueFamilyPresentSupport(
        const vk::Instance& instance,
        const vk::PhysicalDevice& physicalDevice,
        std::uint32_t index
    ) const -> bool = 0;
};
}
