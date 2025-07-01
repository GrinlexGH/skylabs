#include "render_context.hpp"

#include "logging.hpp"

namespace Vulkan {
CRenderContext::CRenderContext(const IVulkanWindow* const window) : m_window(window) {
    CreateInstance();
    SelectPhysicalDevice();
    CreateLogicalDevice();
}

void CRenderContext::CreateInstance() {
    std::unordered_map<const char*, bool> instanceExtensions {
        { vk::KHRGetPhysicalDeviceProperties2ExtensionName, true }
    };

    const std::vector<const char*> requiredExtensions = m_window->GetRequiredInstanceExtensions();
    instanceExtensions.reserve(requiredExtensions.size());

    for (const auto& extension : requiredExtensions) {
        instanceExtensions[extension] = true;
    }

    m_instance = std::make_unique<CInstance>(instanceExtensions);
}

void CRenderContext::SelectPhysicalDevice() {
    m_selectedPhysicalDevice = m_instance->GetSuitablePhysicalDevice(m_window);

    Log::Info("Selected device: {}", std::string_view { m_selectedPhysicalDevice->GetProperties().deviceName });

    REQUEST_REQUIRED_FEATURE(m_selectedPhysicalDevice, samplerAnisotropy);
}

void CRenderContext::CreateLogicalDevice() {
    std::unordered_map<const char*, bool> deviceExtensions {
        // VMA
        { vk::KHRDedicatedAllocationExtensionName, false },
        { vk::KHRBindMemory2ExtensionName, false },
        { vk::KHRMaintenance4ExtensionName, false },
        { vk::KHRMaintenance5ExtensionName, false },
        { vk::EXTMemoryBudgetExtensionName, false },
        { vk::KHRBufferDeviceAddressExtensionName, false },
        { vk::EXTMemoryPriorityExtensionName, false },
        { vk::AMDDeviceCoherentMemoryExtensionName, false },
#ifdef PLATFORM_WINDOWS
        { vk::KHRExternalMemoryWin32ExtensionName, false },
#endif

        { vk::KHRSwapchainExtensionName, true },

#ifdef DEBUG
        { vk::EXTDeviceAddressBindingReportExtensionName, false }
#endif
    };

    // Enable all extensions here
    if (!REQUEST_OPTIONAL_EXT_FEATURE(m_selectedPhysicalDevice, vk::PhysicalDeviceVulkan13Features, dynamicRendering)) {
        deviceExtensions[vk::KHRDynamicRenderingExtensionName] = true;
        REQUEST_REQUIRED_EXT_FEATURE(m_selectedPhysicalDevice, vk::PhysicalDeviceDynamicRenderingFeatures, dynamicRendering);
    }

    m_device = std::make_unique<CDevice>(
        *m_instance,
        *m_selectedPhysicalDevice,
        m_window,
        deviceExtensions
    );
}
}
