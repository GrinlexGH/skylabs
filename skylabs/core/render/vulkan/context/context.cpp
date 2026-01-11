#include <skylabs/core/render/vulkan/context/context.hpp>
#include <skylabs/public/logging.hpp>

#include <ranges>

namespace {
bool EnableDynamicRender(
    const std::uint32_t apiVersion,
    const Vulkan::CPhysicalDevice& gpu,
    Vulkan::CDevice::DeviceFeatures& features,
    std::vector<const char*>& deviceExtensions
) {
    if (apiVersion >= vk::ApiVersion13) {
        features.unlink<vk::PhysicalDeviceDynamicRenderingFeaturesKHR>();
        features.get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering = vk::True;
        return true;
    }

    if (gpu.IsExtensionAvailable(vk::KHRDynamicRenderingExtensionName) && gpu.Features().get<vk::PhysicalDeviceDynamicRenderingFeaturesKHR>().dynamicRendering) {
        features.get<vk::PhysicalDeviceDynamicRenderingFeaturesKHR>().dynamicRendering = vk::True;
        deviceExtensions.emplace_back(vk::KHRDynamicRenderingExtensionName);

        // dependencies
        if (apiVersion < vk::ApiVersion12) {
            deviceExtensions.emplace_back(vk::KHRDepthStencilResolveExtensionName);
            deviceExtensions.emplace_back(vk::KHRCreateRenderpass2ExtensionName);
            if (apiVersion < vk::ApiVersion11) {
                deviceExtensions.emplace_back(vk::KHRMultiviewExtensionName);
                deviceExtensions.emplace_back(vk::KHRMaintenance2ExtensionName);
            }
        }

        return true;
    }

    features.unlink<vk::PhysicalDeviceDynamicRenderingFeaturesKHR>();
    return false;
}

bool EnableSwapchain(
    const std::uint32_t /*apiVersion*/,
    const Vulkan::CPhysicalDevice& gpu,
    Vulkan::CDevice::DeviceFeatures& /*features*/,
    std::vector<const char*>& deviceExtensions
) {
    if (gpu.IsExtensionAvailable(vk::KHRSwapchainExtensionName)) {
        deviceExtensions.emplace_back(vk::KHRSwapchainExtensionName);
        return true;
    }
    return false;
}
}

namespace Vulkan {
CContext::CContext(const IWindow* const window) : m_window(window) {
    CreateInstance();
    CreateDevice();
    m_allocator = CAllocator { *m_instance, m_physicalDevice, m_device };
}

void CContext::CreateInstance() {
    VULKAN_HPP_DEFAULT_DISPATCHER.init();

    std::uint32_t apiVersion = vk::ApiVersion10;
    if (m_context.getDispatcher()->vkEnumerateInstanceVersion) {
        apiVersion = m_context.enumerateInstanceVersion();
        Log::Debug(
            "Available Vulkan version: {}.{}.{}, but anyways I will use Vulkan 1.0", vk::apiVersionMajor(apiVersion), vk::apiVersionMinor(apiVersion),
            vk::apiVersionPatch(apiVersion)
        );

        apiVersion = vk::ApiVersion10;
    }

    std::vector<Utils::CRequestedExtension> instanceExtensions = m_window->GetRequiredInstanceExtensions() |
        std::views::transform([](const char* const ext) -> Utils::CRequestedExtension { return { .m_name = ext, .m_requirement = Utils::Requirement::eRequired }; }) |
        std::ranges::to<std::vector<Utils::CRequestedExtension>>();

    if (apiVersion < vk::ApiVersion11) {
        instanceExtensions.emplace_back(vk::KHRGetPhysicalDeviceProperties2ExtensionName, Utils::Requirement::eRequired);
    }

    m_instance = CInstance { m_context, apiVersion, instanceExtensions };
}

// Guarantees:
// * Graphics & present queue families
// * VK_KHR_swapchain extension
// * SamplerAnisotropy feature
bool CContext::IsDeviceSuitable(const CPhysicalDevice& physicalDeviceInfo) const {
    // Check for graphics and queue families
    bool hasGraphicsQueue = false;
    bool hasPresentQueue = false;

    const std::vector<vk::QueueFamilyProperties2KHR>& queueFamilies = physicalDeviceInfo.QueueFamilies();
    for (uint32_t i = 0; i < queueFamilies.size(); i++) {
        if (queueFamilies.at(i).queueFamilyProperties.queueFlags & vk::QueueFlagBits::eGraphics) hasGraphicsQueue = true;
        if (m_window->IsQueueFamilySupportPresent(*m_instance, *physicalDeviceInfo, i)) hasPresentQueue = true;
        if (hasGraphicsQueue && hasPresentQueue) break;
    }

    if (!hasGraphicsQueue || !hasPresentQueue) return false;

    // Check for swapchain extension
    bool hasSwapchainExtension = false;

    for (const auto& extension : physicalDeviceInfo.AvailableExtensions()) {
        if (std::strcmp(extension.extensionName, vk::KHRSwapchainExtensionName) == 0) {
            hasSwapchainExtension = true;
            break;
        }
    }

    if (!hasSwapchainExtension) return false;

    // Check for sampler anisotropy
    if (!physicalDeviceInfo.Features().get<vk::PhysicalDeviceFeatures2KHR>().features.samplerAnisotropy) return false;

    return true;
}

int CContext::RatePhysicalDevice(const CPhysicalDevice& physicalDeviceInfo) const {
    int score = 0;

    if (!IsDeviceSuitable(physicalDeviceInfo)) return score;

    switch (physicalDeviceInfo.Properties().properties.deviceType) {
        case vk::PhysicalDeviceType::eDiscreteGpu: score += 2000; break;
        case vk::PhysicalDeviceType::eIntegratedGpu: score += 800; break;
        case vk::PhysicalDeviceType::eVirtualGpu: score += 500; break;
        case vk::PhysicalDeviceType::eCpu: score += 200; break;
        case vk::PhysicalDeviceType::eOther: score += 100; break;
    }

    return score;
}

CPhysicalDevice CContext::SelectPhysicalDevice(const vk::raii::PhysicalDevices& physicalDevices) const {
    CPhysicalDevice deviceInfo { nullptr };
    int maxScore = 0;

    for (const auto& device : physicalDevices) {
        CPhysicalDevice currentDeviceInfo { device };
        if (const int score = RatePhysicalDevice(currentDeviceInfo); score > maxScore) {
            maxScore = score;
            deviceInfo = std::move(currentDeviceInfo);
        }
    }

    if (maxScore == 0) {
        throw std::runtime_error("Failed to find a suitable GPU!");
    }

    return deviceInfo;
}

void CContext::CreateDevice() {
    const vk::raii::PhysicalDevices physicalDevices { *m_instance };
    const CPhysicalDevice selectedGPU = SelectPhysicalDevice(physicalDevices);

    const std::uint32_t deviceApiVersion = selectedGPU.Properties().properties.apiVersion;
    const std::uint32_t usingApiVersion = std::min(m_instance.ApiVersion(), deviceApiVersion);
    Log::Debug(
        "Device vulkan api version is {}.{}.{}, minimum version is {}.{}.{}", vk::apiVersionMajor(deviceApiVersion), vk::apiVersionMinor(deviceApiVersion),
        vk::apiVersionPatch(deviceApiVersion), vk::apiVersionMajor(usingApiVersion), vk::apiVersionMinor(usingApiVersion), vk::apiVersionPatch(usingApiVersion)
    );

    std::vector<CDevice::CRequestedFeature> deviceFeatures;
    deviceFeatures.emplace_back(EnableSwapchain, Utils::Requirement::eRequired);
    deviceFeatures.emplace_back(EnableDynamicRender, Utils::Requirement::eOptional);

    m_device = CDevice { m_window, *m_instance, selectedGPU, usingApiVersion, deviceFeatures };
}
}
