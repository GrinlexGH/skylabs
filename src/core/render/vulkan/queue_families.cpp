#include "queue_families.hpp"

#include "vulkan_window.hpp"

namespace Vulkan {
void CQueueFamilies::Init(const vk::Instance instance, const vk::PhysicalDevice physicalDevice, const IVulkanWindow* window) {
    int i = 0;
    for (const auto& queueFamily : physicalDevice.getQueueFamilyProperties()) {
        if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) {
            m_graphics = i;
        }

        if (queueFamily.queueFlags & vk::QueueFlagBits::eTransfer) {
            m_transfer = i;
        }

        if (queueFamily.queueFlags & vk::QueueFlagBits::eCompute) {
            m_compute = i;
        }

        if (window->CheckQueuePresentSupport(instance, physicalDevice, i)) {
            m_present = i;
        }

        if (IsComplete()) {
            break;
        }

        i++;
    }
}
}
