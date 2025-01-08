#include "device.hpp"

#include "extensions/extension_manager.hpp"

#include <set>

namespace Vulkan
{
void CDevice::Initialize(
    const vk::Instance instance,
    const IVulkanWindow* window
) {
    const std::vector requiredExtensions {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_EXT_MEMORY_BUDGET_EXTENSION_NAME
    };

    m_physicalDevice.Pick(instance, window, requiredExtensions);
    m_queueFamilies.Init(instance, m_physicalDevice.GetHandle(), window);
    Create(requiredExtensions);
    m_queues.Init(m_handle, m_queueFamilies);
    CreateAllocator(instance);
}

CDevice::~CDevice() {
    if (m_handle) {
        m_handle.destroy();
    }
}

void CDevice::Create(
    const std::vector<const char*>& requiredExtensions
) {
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

void CDevice::CreateAllocator(
    const vk::Instance instance
) {
    vma::AllocatorCreateInfo allocatorCreateInfo;
    allocatorCreateInfo.flags = vma::AllocatorCreateFlagBits::eExtMemoryBudget;
    allocatorCreateInfo.vulkanApiVersion = vk::ApiVersion13;
    allocatorCreateInfo.physicalDevice = m_physicalDevice.GetHandle();
    allocatorCreateInfo.device = m_handle;
    allocatorCreateInfo.instance = instance;

    const vma::VulkanFunctions vulkanFunctions = vma::functionsFromDispatcher();
    allocatorCreateInfo.pVulkanFunctions = &vulkanFunctions;

    m_allocator = createAllocator(allocatorCreateInfo);
}
}
