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
    // instanceExtensions[name] -> isRequired
    std::unordered_map<std::string_view, bool> instanceExtensions;

    const std::span<const char* const> required = m_window->GetRequiredInstanceExtensions();
    instanceExtensions.reserve(required.size() + 1);

    instanceExtensions[vk::KHRGetPhysicalDeviceProperties2ExtensionName] = true;

    for (const std::string_view ext : required) {
        instanceExtensions[ext] = true;
    }

    m_instance = CInstance { instanceExtensions, {} };
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
    // deviceExtensions[name] -> isRequired
    std::unordered_map<std::string_view, bool> deviceExtensions;
    deviceExtensions.reserve(15);

    // VMA
    deviceExtensions[vk::KHRDedicatedAllocationExtensionName] = false;
    deviceExtensions[vk::KHRBindMemory2ExtensionName] = false;
    deviceExtensions[vk::KHRMaintenance4ExtensionName] = false;
    deviceExtensions[vk::KHRMaintenance5ExtensionName] = false;
    deviceExtensions[vk::EXTMemoryBudgetExtensionName] = false;
    deviceExtensions[vk::KHRBufferDeviceAddressExtensionName] = false;
    deviceExtensions[vk::EXTMemoryPriorityExtensionName] = false;
    deviceExtensions[vk::AMDDeviceCoherentMemoryExtensionName] = false;

    deviceExtensions[vk::KHRSwapchainExtensionName] = true;

#ifdef VK_USE_PLATFORM_WIN32_KHR
    deviceExtensions[vk::KHRExternalMemoryWin32ExtensionName] = false;
#endif

#ifdef DEBUG
    deviceExtensions[vk::EXTDeviceAddressBindingReportExtensionName] = false;
#endif

    // Enable all extensions here
    REQUEST_REQUIRED_FEATURE(m_selectedPhysicalDevice, samplerAnisotropy);

    if (!REQUEST_OPTIONAL_EXT_FEATURE(m_selectedPhysicalDevice, vk::PhysicalDeviceVulkan13Features, dynamicRendering)) {
        deviceExtensions[vk::KHRDynamicRenderingExtensionName] = false;
        REQUEST_OPTIONAL_EXT_FEATURE(m_selectedPhysicalDevice, vk::PhysicalDeviceDynamicRenderingFeatures, dynamicRendering);
    }

    if (!REQUEST_OPTIONAL_EXT_FEATURE(m_selectedPhysicalDevice, vk::PhysicalDeviceVulkan13Features, synchronization2)) {
        deviceExtensions[vk::KHRSynchronization2ExtensionName] = true;
        REQUEST_REQUIRED_EXT_FEATURE(m_selectedPhysicalDevice, vk::PhysicalDeviceSynchronization2Features, synchronization2);
    }

    if (!REQUEST_OPTIONAL_EXT_FEATURE(m_selectedPhysicalDevice, vk::PhysicalDeviceVulkan12Features, bufferDeviceAddress)) {
        deviceExtensions[vk::KHRBufferDeviceAddressExtensionName] = true;
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
