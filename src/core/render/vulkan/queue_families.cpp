#include "queue_families.hpp"

#include "vulkan_window.hpp"

namespace Vulkan
{
CQueueFamilies CQueueFamilies::Find(
    const vk::Instance instance,
    const vk::PhysicalDevice physicalDevice,
    const IVulkanWindow* window
) {
    CQueueFamilies indices;

    int i = 0;
    for (const auto& queueFamily : physicalDevice.getQueueFamilyProperties()) {
        if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) {
            indices.m_graphics = i;
        }

        if (queueFamily.queueFlags & vk::QueueFlagBits::eTransfer) {
            indices.m_transfer = i;
        }

        if (queueFamily.queueFlags & vk::QueueFlagBits::eCompute) {
            indices.m_compute = i;
        }

        if (window->GetQueuePresentSupport(instance, physicalDevice, i)) {
            indices.m_present = i;
        }

        if (indices.IsComplete()) {
            break;
        }

        i++;
    }

    return indices;
}
}
