#pragma once
#include "vulkan.hpp"
#include "vulkan_window.hpp"

#include <optional>

namespace Vulkan
{
class CQueueFamilies
{
public:
    [[nodiscard]] bool IsComplete() const {
        return m_graphics.has_value() && m_present.has_value() && m_transfer.has_value() && m_compute.has_value();
    }

    static CQueueFamilies Find(
        vk::Instance instance,
        vk::PhysicalDevice physicalDevice,
        const IVulkanWindow* window
    );

    std::optional<int> m_graphics = 0;
    std::optional<int> m_present = 0;
    std::optional<int> m_transfer = 0;
    std::optional<int> m_compute = 0;
};
}
