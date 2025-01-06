#include "physical_device.hpp"

#include "console.hpp"
#include "queue_families.hpp"
#include "vulkan_window.hpp"
#include "extensions/extension_manager.hpp"

namespace
{
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
    }

    return 0;
}

bool CheckExtensionSupport(
    const vk::PhysicalDevice physicalDevice,
    const std::vector<const char*>& requiredExtensions
) {
    const std::vector<vk::ExtensionProperties> availableExtensions = physicalDevice.enumerateDeviceExtensionProperties();

    std::vector<const char*> missingExtensions;

    for (const auto extension : requiredExtensions) {
        if (!HasExtension(availableExtensions, extension)) {
            missingExtensions.push_back(extension);
        }
    }

    if (!missingExtensions.empty()) {
        std::string message = "Required vulkan extensions not found:\n";
        for (const auto ext : missingExtensions) {
            message.append(ext);
            message.append("\n");
        }
        Msg << message;
        return false;
    }

    Msg << "All required vulkan extensions for this device were found!";

    return true;
}

bool CheckQueueSupport(
    const vk::Instance instance,
    const vk::PhysicalDevice physicalDevice,
    const IVulkanWindow* window
) {
    if (!Vulkan::FindQueueFamilies(instance, physicalDevice, window).IsComplete()) {
        return false;
    }

    Msg << "Device has all required queue families!";

    return true;
}

bool IsDeviceSuitable(
    const vk::Instance instance,
    const vk::PhysicalDevice physicalDevice,
    const std::vector<const char*>& requiredExtensions,
    const IVulkanWindow* window
) {
    if (!CheckExtensionSupport(physicalDevice, requiredExtensions)) {
        return false;
    }

    if (!CheckQueueSupport(instance, physicalDevice, window)) {
        return false;
    }

    return true;
}
}

namespace Vulkan
{
void CPhysicalDevice::Pick(
    const vk::Instance instance,
    const std::vector<const char*>& requiredExtensions,
    const IVulkanWindow* window
) {
    const std::vector<vk::PhysicalDevice> physicalDevices = instance.enumeratePhysicalDevices();

    int deviceTypeScore = 0;
    for (vk::PhysicalDevice physicalDevice : physicalDevices) {
        const vk::PhysicalDeviceProperties deviceProperties = physicalDevice.getProperties();
        Msg("Found device: {}", static_cast<const char*>(deviceProperties.deviceName));

        if (IsDeviceSuitable(instance, physicalDevice, requiredExtensions, window)) {
            const int optionScore = GetDeviceTypeScore(deviceProperties.deviceType);

            if (optionScore > deviceTypeScore) {
                m_handle = physicalDevice;
                deviceTypeScore = optionScore;
            }
        }
    }

    if (m_handle == VK_NULL_HANDLE) {
        throw std::runtime_error("No suitable GPU was found!");
    }
}
}
