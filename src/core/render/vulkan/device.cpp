#include "device.hpp"

#include "extensions/extension_manager.hpp"

#include <set>

namespace Vulkan
{
void CDevice::Create(
    const vk::Instance instance,
    const vk::PhysicalDevice physicalDevice,
    const std::vector<const char*>& requiredExtensions,
    const IVulkanWindow* window
) {
    m_queueFamilies.Init(instance, physicalDevice, window);

    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies {
        m_queueFamilies.m_graphics.value(),
        m_queueFamilies.m_present.value(),
        m_queueFamilies.m_transfer.value(),
        m_queueFamilies.m_compute.value()
    };

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        vk::DeviceQueueCreateInfo queueCreateInfo {};
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    //==========
    // assume when picking physical device, all required extensions are pesent
    std::vector<const char*> enabledExtensions;
    enabledExtensions.reserve(requiredExtensions.size());
    for (const char* extension : requiredExtensions) {
        enabledExtensions.push_back(extension);
    }

    //==========
    vk::PhysicalDeviceFeatures requestedDeviceFeatures {};
    requestedDeviceFeatures.samplerAnisotropy = true;

    //==========
    vk::DeviceCreateInfo deviceCreateInfo;
    deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
    deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    deviceCreateInfo.pEnabledFeatures = &requestedDeviceFeatures;
    deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(enabledExtensions.size());
    deviceCreateInfo.ppEnabledExtensionNames = enabledExtensions.data();

    m_handle = physicalDevice.createDevice(deviceCreateInfo);
}

CDevice::~CDevice() {
    if (m_handle) {
        m_handle.destroy();
    }
}
}
