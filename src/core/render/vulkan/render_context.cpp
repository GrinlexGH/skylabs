#include "render_context.hpp"

#include "logging.hpp"

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
    }

    return 0;
}
}

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

bool CRenderContext::IsDeviceSuitable(const CPhysicalDevice* const physicalDevice) const {
    bool hasPresentQueue = false;
    bool hasGraphicsQueue = false;

    for (std::uint32_t i = 0; const auto& queue : physicalDevice->GetQueueFamilies()) {
        if (m_window->CheckQueuePresentSupport(m_instance->GetHandle(), physicalDevice->GetHandle(), i)) {
            hasPresentQueue = true;
        }

        if (queue.queueFlags & vk::QueueFlagBits::eGraphics) {
            hasGraphicsQueue = true;
        }

        if (hasPresentQueue && hasGraphicsQueue) {
            return true;
        }

        ++i;
    }

    return false;
}

CPhysicalDevice* CRenderContext::GetSuitablePhysicalDevice() const {
    CPhysicalDevice* selectedDevice = nullptr;
    const std::vector<std::unique_ptr<CPhysicalDevice>>& physicalDevices = m_instance->GetPhysicalDevices();

    int deviceTypeScore = 0;
    for (const auto& physicalDevice : physicalDevices) {
        if (IsDeviceSuitable(physicalDevice.get())) {
            if (const int optionScore = GetDeviceTypeScore(physicalDevice->GetProperties().deviceType); optionScore > deviceTypeScore) {
                selectedDevice = physicalDevice.get();
                deviceTypeScore = optionScore;
            }
        }
    }

    if (selectedDevice == nullptr) {
        Log::Warning("No suitable GPU was found! Picking default GPU: {}", *physicalDevices[0]->GetProperties().deviceName);
        return physicalDevices[0].get();
    }

    return selectedDevice;
}

void CRenderContext::SelectPhysicalDevice() {
    m_selectedPhysicalDevice = GetSuitablePhysicalDevice();

    Log::Info("Selected device: {}", std::string_view { m_selectedPhysicalDevice->GetProperties().deviceName });
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
    REQUEST_REQUIRED_FEATURE(m_selectedPhysicalDevice, samplerAnisotropy);

    if (!REQUEST_OPTIONAL_EXT_FEATURE(m_selectedPhysicalDevice, vk::PhysicalDeviceVulkan13Features, dynamicRendering)) {
        deviceExtensions[vk::KHRDynamicRenderingExtensionName] = false;
        REQUEST_OPTIONAL_EXT_FEATURE(m_selectedPhysicalDevice, vk::PhysicalDeviceDynamicRenderingFeatures, dynamicRendering);
    }

    m_device = std::make_unique<CDevice>(
        *m_instance,
        *m_selectedPhysicalDevice,
        m_window,
        deviceExtensions
    );
}
}
