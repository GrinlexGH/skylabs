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

private:
    vk::PhysicalDevice m_handle;
};
}
