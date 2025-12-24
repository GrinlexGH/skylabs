#include <skylabs/core/render/vulkan/context/context.hpp>

#include <skylabs/public/logging.hpp>

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
CContext::CContext(const IWindow* const window) : m_window(window) {
    CreateInstance();
    SelectPhysicalDevice();
    CreateLogicalDevice();
    CreateAllocator();
}

auto CContext::CreateInstance() -> void {
    std::vector<RequestedExtension> instanceExtensions;

    const std::span<const char* const> required = m_window->GetRequiredInstanceExtensions();
    instanceExtensions.reserve(required.size() + 1);

    instanceExtensions.emplace_back(
        vk::KHRGetPhysicalDeviceProperties2ExtensionName,
        ExtensionRequirement::Required
    );

    for (const std::string_view ext : required) {
        instanceExtensions.emplace_back(ext, ExtensionRequirement::Required);
    }

    std::ranges::sort(instanceExtensions);
    instanceExtensions.erase(std::ranges::unique(instanceExtensions).begin(), instanceExtensions.end());

    m_instance = CInstance { instanceExtensions };
}

auto CContext::IsDeviceSuitable(const CPhysicalDevice& physicalDevice) const -> bool {
    bool hasPresentQueue = false;
    bool hasGraphicsQueue = false;

    for (std::uint32_t i = 0; const auto& queue : physicalDevice.GetQueueFamilies()) {
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
    std::vector<RequestedExtension> deviceExtensions;
    deviceExtensions.reserve(15);

    // VMA
    deviceExtensions.emplace_back(vk::KHRDedicatedAllocationExtensionName, ExtensionRequirement::Optional);
    deviceExtensions.emplace_back(vk::KHRBindMemory2ExtensionName, ExtensionRequirement::Optional);
    deviceExtensions.emplace_back(vk::KHRMaintenance4ExtensionName, ExtensionRequirement::Optional);
    deviceExtensions.emplace_back(vk::KHRMaintenance5ExtensionName, ExtensionRequirement::Optional);
    deviceExtensions.emplace_back(vk::EXTMemoryBudgetExtensionName, ExtensionRequirement::Optional);
    deviceExtensions.emplace_back(vk::KHRBufferDeviceAddressExtensionName, ExtensionRequirement::Optional);
    deviceExtensions.emplace_back(vk::EXTMemoryPriorityExtensionName, ExtensionRequirement::Optional);
    deviceExtensions.emplace_back(vk::AMDDeviceCoherentMemoryExtensionName, ExtensionRequirement::Optional);

    deviceExtensions.emplace_back(vk::KHRSwapchainExtensionName, ExtensionRequirement::Required);

#ifdef VK_USE_PLATFORM_WIN32_KHR
    deviceExtensions.emplace_back(vk::KHRExternalMemoryWin32ExtensionName, ExtensionRequirement::Optional);
#endif

#ifdef DEBUG
    deviceExtensions.emplace_back(vk::EXTDeviceAddressBindingReportExtensionName, ExtensionRequirement::Optional);
#endif

    // Enable all extensions here
    REQUEST_REQUIRED_FEATURE(m_selectedPhysicalDevice, samplerAnisotropy);

    if (!REQUEST_OPTIONAL_EXT_FEATURE(m_selectedPhysicalDevice, vk::PhysicalDeviceVulkan13Features, dynamicRendering)) {
        deviceExtensions.emplace_back(vk::KHRDynamicRenderingExtensionName, ExtensionRequirement::Optional);
        REQUEST_OPTIONAL_EXT_FEATURE(m_selectedPhysicalDevice, vk::PhysicalDeviceDynamicRenderingFeatures, dynamicRendering);
    }

    if (!REQUEST_OPTIONAL_EXT_FEATURE(m_selectedPhysicalDevice, vk::PhysicalDeviceVulkan13Features, synchronization2)) {
        deviceExtensions.emplace_back(vk::KHRSynchronization2ExtensionName, ExtensionRequirement::Required);
        REQUEST_REQUIRED_EXT_FEATURE(m_selectedPhysicalDevice, vk::PhysicalDeviceSynchronization2Features, synchronization2);
    }

    if (!REQUEST_OPTIONAL_EXT_FEATURE(m_selectedPhysicalDevice, vk::PhysicalDeviceVulkan12Features, bufferDeviceAddress)) {
        deviceExtensions.emplace_back(vk::KHRBufferDeviceAddressExtensionName, ExtensionRequirement::Required);
        REQUEST_REQUIRED_EXT_FEATURE(m_selectedPhysicalDevice, vk::PhysicalDeviceBufferDeviceAddressFeatures, bufferDeviceAddress);
    }

    m_device = CDevice {
        m_instance,
        *m_selectedPhysicalDevice,
        m_window,
        deviceExtensions
    };
}

auto CContext::CreateAllocator() -> void {
    m_allocator = CAllocator { m_instance, m_selectedPhysicalDevice->GetHandle(), m_device };
}
}
