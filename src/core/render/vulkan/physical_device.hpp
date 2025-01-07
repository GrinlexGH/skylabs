#pragma once
#include "vulkan.hpp"
#include "vulkan_window.hpp"

namespace Vulkan
{
class CPhysicalDevice
{
public:
    void Pick(
        vk::Instance instance,
        const std::vector<const char*>& requiredExtensions,
        const IVulkanWindow* window
    );
    [[nodiscard]] vk::PhysicalDevice GetHandle() const { return m_handle; }

private:
    vk::PhysicalDevice m_handle;
};
}
