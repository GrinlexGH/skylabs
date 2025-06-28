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
        { VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME, true }
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
        { VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME, false },
        { VK_KHR_BIND_MEMORY_2_EXTENSION_NAME, false },
        { VK_KHR_MAINTENANCE_4_EXTENSION_NAME, false },
        { VK_KHR_MAINTENANCE_5_EXTENSION_NAME, false },
        { VK_EXT_MEMORY_BUDGET_EXTENSION_NAME, false },
        { VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME, false },
        { VK_EXT_MEMORY_PRIORITY_EXTENSION_NAME, false },
        { VK_AMD_DEVICE_COHERENT_MEMORY_EXTENSION_NAME, false },
#ifdef PLATFORM_WINDOWS
        { VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME, false },
#endif

        { VK_KHR_SWAPCHAIN_EXTENSION_NAME, true },

#ifdef DEBUG
        { VK_EXT_DEVICE_ADDRESS_BINDING_REPORT_EXTENSION_NAME, false }
#endif
    };

    // Enable all extensions here
    if (!REQUEST_OPTIONAL_EXT_FEATURE(m_selectedPhysicalDevice, vk::PhysicalDeviceVulkan13Features, dynamicRendering)) {
        deviceExtensions[VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME] = true;
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
