#include "device.hpp"

#include <set>

namespace Vulkan {
CDevice::CDevice(
    const CInstance& instance,
    const IVulkanWindow* window
) {
    m_physicalDevice.Pick(instance, window);

    //m_queueFamilies.Init(instance, m_physicalDevice.GetHandle(), window);

    //Create(requiredExtensions);

    //m_queues.Init(m_handle, m_queueFamilies);

    //m_swapchain.Init(m_physicalDevice.GetHandle(), m_handle, window->GetSurface());

    //m_allocator.Create(instance, m_physicalDevice.GetHandle(), m_handle);
}

CDevice::~CDevice() {
    if (m_handle) {
        m_handle.destroy();
    }
}

void CDevice::Create(const std::vector<const char*>& requiredExtensions) {
    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies {
        *m_queueFamilies.m_graphics,
        *m_queueFamilies.m_present,
        *m_queueFamilies.m_transfer,
        *m_queueFamilies.m_compute
    };

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        vk::DeviceQueueCreateInfo queueCreateInfo {};
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    std::vector<const char*> enabledExtensions;
    enabledExtensions.reserve(requiredExtensions.size());
    for (const char* extension : requiredExtensions) {
        enabledExtensions.push_back(extension);
    }

    vk::PhysicalDeviceFeatures requestedDeviceFeatures {};
    requestedDeviceFeatures.samplerAnisotropy = true;

    vk::DeviceCreateInfo deviceCreateInfo;
    deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
    deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    deviceCreateInfo.pEnabledFeatures = &requestedDeviceFeatures;
    deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(enabledExtensions.size());
    deviceCreateInfo.ppEnabledExtensionNames = enabledExtensions.data();

    m_handle = m_physicalDevice.GetHandle().createDevice(deviceCreateInfo);
}
}
