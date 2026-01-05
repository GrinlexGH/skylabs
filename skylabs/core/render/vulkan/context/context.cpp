#include <skylabs/core/render/vulkan/context/context.hpp>

#include <skylabs/public/logging.hpp>

namespace {
int DeviceTypeScore(const vk::PhysicalDeviceType type) {
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
    std::unreachable();
}
}

namespace Vulkan {
CContext::CContext(const IWindow* const window) : m_window(window) {
    CreateInstance();
    SelectPhysicalDevice();
    CreateLogicalDevice();
    CreateAllocator();
}

void CContext::CreateInstance() {
    std::vector<Utils::CRequestedExtension> instanceExtensions;

    const std::span<const char* const> required = m_window->GetRequiredInstanceExtensions();
    instanceExtensions.reserve(required.size() + 1);

    for (const std::string_view ext : required) {
        instanceExtensions.emplace_back(ext, Utils::ExtensionRequirement::Required);
    }

    instanceExtensions.emplace_back(vk::KHRGetPhysicalDeviceProperties2ExtensionName, Utils::ExtensionRequirement::Required, vk::ApiVersion11);

    std::ranges::sort(instanceExtensions);
    instanceExtensions.erase(std::ranges::unique(instanceExtensions).begin(), instanceExtensions.end());

    m_instance = CInstance { instanceExtensions };
}

bool CContext::IsDeviceSuitable(const CPhysicalDevice& physicalDevice) const {
    bool hasPresentQueue = false;
    bool hasGraphicsQueue = false;

    for (std::uint32_t i = 0; const auto& queue : physicalDevice.QueueFamilies()) {
        if (m_window->IsQueueFamilyPresentSupport(*m_instance, *physicalDevice, i)) {
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

CPhysicalDevice* CContext::SelectSuitablePhysicalDevice() {
    CPhysicalDevice* selectedDevice = nullptr;
    std::vector<CPhysicalDevice>& physicalDevices = m_instance.PhysicalDevices();

    int deviceTypeScore = 0;
    for (auto& physicalDevice : physicalDevices) {
        if (IsDeviceSuitable(physicalDevice)) {
            if (const int optionScore = DeviceTypeScore(physicalDevice.Properties().deviceType); optionScore > deviceTypeScore) {
                selectedDevice = &physicalDevice;
                deviceTypeScore = optionScore;
            }
        }
    }

    if (selectedDevice == nullptr) {
        Log::Warning("No suitable GPU was found! Picking first GPU: {}", *physicalDevices.at(0).Properties().deviceName);
        return physicalDevices.data();
    }

    return selectedDevice;
}

void CContext::SelectPhysicalDevice() {
    m_selectedPhysicalDevice = SelectSuitablePhysicalDevice();

    Log::Info("Selected device: {}", std::string_view { m_selectedPhysicalDevice->Properties().deviceName });
}

void CContext::CreateLogicalDevice() {
    std::vector<Utils::CRequestedExtension> deviceExtensions;
    deviceExtensions.reserve(15);

    deviceExtensions.emplace_back(vk::KHRSwapchainExtensionName, Utils::ExtensionRequirement::Required);

    // VMA
    deviceExtensions.emplace_back(vk::KHRDedicatedAllocationExtensionName, Utils::ExtensionRequirement::Optional, vk::ApiVersion11);
    deviceExtensions.emplace_back(vk::KHRGetMemoryRequirements2ExtensionName, Utils::ExtensionRequirement::Optional, vk::ApiVersion11);

    deviceExtensions.emplace_back(vk::KHRBindMemory2ExtensionName, Utils::ExtensionRequirement::Optional, vk::ApiVersion11);
    deviceExtensions.emplace_back(vk::KHRMaintenance4ExtensionName, Utils::ExtensionRequirement::Optional, vk::ApiVersion13);
    deviceExtensions.emplace_back(vk::EXTMemoryBudgetExtensionName, Utils::ExtensionRequirement::Optional);
    deviceExtensions.emplace_back(vk::EXTMemoryPriorityExtensionName, Utils::ExtensionRequirement::Optional);
    deviceExtensions.emplace_back(vk::AMDDeviceCoherentMemoryExtensionName, Utils::ExtensionRequirement::Optional);

#ifdef VK_USE_PLATFORM_WIN32_KHR
    deviceExtensions.emplace_back(vk::KHRExternalMemoryWin32ExtensionName, Utils::ExtensionRequirement::Optional);
#endif

#ifdef DEBUG
    deviceExtensions.emplace_back(vk::EXTDeviceAddressBindingReportExtensionName, Utils::ExtensionRequirement::Optional);
#endif

    // Enable all extensions here
    REQUEST_REQUIRED_FEATURE(m_selectedPhysicalDevice, samplerAnisotropy);

    deviceExtensions.emplace_back(vk::KHRMaintenance5ExtensionName, Utils::ExtensionRequirement::Optional, vk::ApiVersion14);
    if (m_instance.ApiVersion() < vk::ApiVersion13 || !REQUEST_OPTIONAL_EXT_FEATURE(m_selectedPhysicalDevice, vk::PhysicalDeviceVulkan13Features, dynamicRendering)) {
        deviceExtensions.emplace_back(vk::KHRDynamicRenderingExtensionName, Utils::ExtensionRequirement::Optional);
        deviceExtensions.emplace_back(vk::KHRDepthStencilResolveExtensionName, Utils::ExtensionRequirement::Optional);
        deviceExtensions.emplace_back(vk::KHRCreateRenderpass2ExtensionName, Utils::ExtensionRequirement::Optional);
        REQUEST_OPTIONAL_EXT_FEATURE(m_selectedPhysicalDevice, vk::PhysicalDeviceDynamicRenderingFeatures, dynamicRendering);
    }

    if (m_instance.ApiVersion() < vk::ApiVersion13 || !REQUEST_OPTIONAL_EXT_FEATURE(m_selectedPhysicalDevice, vk::PhysicalDeviceVulkan13Features, synchronization2)) {
        deviceExtensions.emplace_back(vk::KHRSynchronization2ExtensionName, Utils::ExtensionRequirement::Required);
        REQUEST_REQUIRED_EXT_FEATURE(m_selectedPhysicalDevice, vk::PhysicalDeviceSynchronization2Features, synchronization2);
    }

    deviceExtensions.emplace_back(vk::KHRDeviceGroupExtensionName, Utils::ExtensionRequirement::Optional, vk::ApiVersion11);
    if (m_instance.ApiVersion() < vk::ApiVersion12 || !REQUEST_OPTIONAL_EXT_FEATURE(m_selectedPhysicalDevice, vk::PhysicalDeviceVulkan12Features, bufferDeviceAddress)) {
        deviceExtensions.emplace_back(vk::KHRBufferDeviceAddressExtensionName, Utils::ExtensionRequirement::Required);
        REQUEST_REQUIRED_EXT_FEATURE(m_selectedPhysicalDevice, vk::PhysicalDeviceBufferDeviceAddressFeatures, bufferDeviceAddress);
    }

    std::ranges::sort(deviceExtensions);
    deviceExtensions.erase(std::ranges::unique(deviceExtensions).begin(), deviceExtensions.end());

    m_device = CDevice {
        m_instance,
        *m_selectedPhysicalDevice,
        m_window,
        deviceExtensions
    };
}

void CContext::CreateAllocator() {
    m_allocator = CAllocator { m_instance, m_selectedPhysicalDevice->Handle(), m_device };
}
}
