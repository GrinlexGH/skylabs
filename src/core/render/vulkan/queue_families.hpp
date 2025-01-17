#pragma once
#include "vulkan.hpp"
#include "vulkan_window.hpp"

#include <optional>

namespace Vulkan {
class CQueueFamilies
{
public:
    void Init(vk::Instance instance, vk::PhysicalDevice physicalDevice, const IVulkanWindow* window);

    [[nodiscard]] bool IsComplete() const {
        return m_graphics.has_value() && m_present.has_value() && m_transfer.has_value() && m_compute.has_value();
    }

    std::optional<unsigned int> m_graphics;
    std::optional<unsigned int> m_present;
    std::optional<unsigned int> m_transfer;
    std::optional<unsigned int> m_compute;
};
}
