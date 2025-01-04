#include "physical_device.hpp"

#include "console.hpp"
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

bool IsDeviceSuitable(
    const vk::PhysicalDevice physicalDevice,
    const std::vector<const char*>& requiredExtensions
) {
    if (!CheckExtensionSupport(physicalDevice, requiredExtensions)) {
        return false;
    }

    return true;
}
}

namespace Vulkan
{
void CPhysicalDevice::Pick(
    vk::Instance instance,
    const std::vector<const char*>& requiredExtensions
) {
    const std::vector<vk::PhysicalDevice> physicalDevices = instance.enumeratePhysicalDevices();

    int deviceTypeScore = 0;
    for (vk::PhysicalDevice physicalDevice : physicalDevices) {
        const vk::PhysicalDeviceProperties deviceProperties = physicalDevice.getProperties();

        if (IsDeviceSuitable(physicalDevice, requiredExtensions)) {
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
