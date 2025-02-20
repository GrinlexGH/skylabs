#pragma once
#include "vulkan.hpp"
#include "vulkan_window.hpp"

namespace Vulkan {
class CPhysicalDevice
{
public:
    void Pick(const vk::Instance& instance, const IVulkanWindow* window, const std::vector<const char*>& requiredExtensions);

    [[nodiscard]] vk::PhysicalDevice GetHandle() const { return m_handle; }

private:
    vk::PhysicalDevice m_handle;
};
}
