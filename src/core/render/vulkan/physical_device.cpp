#include "physical_device.hpp"

#include "console.hpp"
#include "queue_families.hpp"
#include "vulkan_window.hpp"

namespace {
int GetDeviceTypeScore(const vk::PhysicalDeviceType type) {
    switch (type) {
        case vk::PhysicalDeviceType::eDiscreteGpu:
            return 5;
        case vk::PhysicalDeviceType::eIntegratedGpu:
            return 4;
        case vk::PhysicalDeviceType::eVirtualGpu:
            return 3;
        case vk::PhysicalDeviceType::eCpu:
            return 2;
        case vk::PhysicalDeviceType::eOther:
            return 1;
        default: return 0;
    }
}

bool IsDeviceSuitable(
    const vk::Instance& instance,
    const vk::PhysicalDevice& physicalDevice,
    const IVulkanWindow* window
) {
    for (int i = 0; i < physicalDevice.getQueueFamilyProperties().size(); ++i) {
        if (window->CheckQueuePresentSupport(instance, physicalDevice, i)) {
            return true;
        }
    }

    return false;
}
}

namespace Vulkan {
void CPhysicalDevice::Pick(
    const CInstance& instance,
    const IVulkanWindow* window
) {
    static const std::vector<vk::PhysicalDevice> physicalDevices = instance.GetHandle().enumeratePhysicalDevices();

    int deviceTypeScore = 0;
    for (vk::PhysicalDevice physicalDevice : physicalDevices) {
        const vk::PhysicalDeviceProperties deviceProperties = physicalDevice.getProperties();

        Msg("Found device: {}", static_cast<const char*>(deviceProperties.deviceName));

        // Prefer discrete gpu
        const int optionScore = GetDeviceTypeScore(deviceProperties.deviceType);
        if (IsDeviceSuitable(instance.GetHandle(), physicalDevice, window)) {
            if (optionScore > deviceTypeScore) {
                m_handle = physicalDevice;
                deviceTypeScore = optionScore;
            }
        }
    }

    if (m_handle == VK_NULL_HANDLE) {
        throw std::runtime_error("No suitable GPU was found!");
    }

    Set(m_handle);
}

void CPhysicalDevice::Set(const vk::PhysicalDevice& physicalDevice) {
    m_handle = physicalDevice;
    m_properties = m_handle.getProperties();
    m_features = m_handle.getFeatures();
    m_queueFamilyProperties = m_handle.getQueueFamilyProperties();
}
}
