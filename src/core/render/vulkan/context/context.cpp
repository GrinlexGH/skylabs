#include "context.hpp"

#include "logging.hpp"

namespace {
auto GetDeviceTypeScore(const vk::PhysicalDeviceType type) -> int {
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
CContext::CContext(std::nullptr_t) {}

CContext::CContext(const IWindow* const window) : m_window(window) {
    CreateInstance();
    SelectPhysicalDevice();
    CreateLogicalDevice();
}

auto CContext::CreateInstance() -> void {
    std::unordered_map<const char*, bool> instanceExtensions {
        { vk::KHRGetPhysicalDeviceProperties2ExtensionName, true }
    };

    const std::vector<const char*> requiredExtensions = m_window->GetRequiredInstanceExtensions();
    instanceExtensions.reserve(requiredExtensions.size());

    for (const auto& extension : requiredExtensions) {
        instanceExtensions[extension] = true;
    }

    m_instance = CInstance { instanceExtensions };
}

auto CContext::IsDeviceSuitable(const CPhysicalDevice& physicalDevice) const -> bool {
    bool hasPresentQueue = false;
    bool hasGraphicsQueue = false;

    for (std::uint32_t i = 0; const auto& queue : physicalDevice.GetQueueFamilies()) {
        if (m_window->IsQueueFamilyPresentSupport(m_instance.GetHandle(), physicalDevice.GetHandle(), i)) {
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

auto CContext::GetSuitablePhysicalDevice() -> CPhysicalDevice* {
    CPhysicalDevice* selectedDevice = nullptr;
    std::vector<CPhysicalDevice>& physicalDevices = m_instance.GetPhysicalDevices();

    int deviceTypeScore = 0;
    for (auto& physicalDevice : physicalDevices) {
        if (IsDeviceSuitable(physicalDevice)) {
            if (const int optionScore = GetDeviceTypeScore(physicalDevice.GetProperties().deviceType); optionScore > deviceTypeScore) {
                selectedDevice = &physicalDevice;
                deviceTypeScore = optionScore;
            }
        }
    }

    if (selectedDevice == nullptr) {
        Log::Warning("No suitable GPU was found! Picking first GPU: {}", *physicalDevices[0].GetProperties().deviceName);
        return physicalDevices.data();
    }

    return selectedDevice;
}

auto CContext::SelectPhysicalDevice() -> void {
    m_selectedPhysicalDevice = GetSuitablePhysicalDevice();

    Log::Info("Selected device: {}", std::string_view { m_selectedPhysicalDevice->GetProperties().deviceName });
}

auto CContext::CreateLogicalDevice() -> void {
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

    m_device = CDevice {
        m_instance,
        *m_selectedPhysicalDevice,
        m_window,
        deviceExtensions
    };
}
}
