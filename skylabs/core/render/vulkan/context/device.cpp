#include <skylabs/core/render/vulkan/context/device.hpp>
#include <skylabs/core/render/vulkan/context/extensions.hpp>

#include <fmt/ranges.h>

struct QueueFamilyIndices
{
    std::uint32_t m_graphicsFamily = 0;
    std::uint32_t m_presentFamily = 0;
    std::uint32_t m_computeFamily = 0;
};

namespace {
template<typename T>
void RequireFeature(const T& available, T& target, vk::Bool32 T::*member, const char* name) {
    if (available.*member == vk::True) {
        target.*member = vk::True;
    } else {
        throw std::runtime_error(std::string("Device doesn't support ") + name);
    }
}

template<typename T>
void OptionalFeature(const T& available, T& target, vk::Bool32 T::*member, bool& capFlag) {
    if (available.*member == vk::True) {
        target.*member = vk::True;
        capFlag = true;
    }
}

std::unordered_map<std::string, bool> RequestExtensions() {
    std::unordered_map<std::string, bool> requestedExtensions;
    auto requestExtension = [&](const char* name, bool required = false) { requestedExtensions.try_emplace(name, required); };

    requestExtension(vk::KHRSwapchainExtensionName, true);
    requestExtension(vk::KHRMaintenance5ExtensionName, false);

    return requestedExtensions;
}

QueueFamilyIndices GetQueueFamilies(const IWindow* window, const vk::Instance instance, const Vulkan::CPhysicalDevice& gpu) {
    std::optional<std::uint32_t> graphicsFamily;
    std::optional<std::uint32_t> presentFamily;
    std::optional<std::uint32_t> computeFamily;
    const std::vector<vk::QueueFamilyProperties>& queueFamilies = gpu->getQueueFamilyProperties();

    for (std::uint32_t i = 0; i < queueFamilies.size(); ++i) {
        if (!graphicsFamily.has_value() && queueFamilies[i].queueFlags & vk::QueueFlagBits::eGraphics) {
            graphicsFamily.emplace(i);
        }

        // Graphics queue guarantees compute
        if (!computeFamily.has_value() && queueFamilies[i].queueFlags & vk::QueueFlagBits::eCompute) {
            computeFamily.emplace(i);
        }

        if (!presentFamily.has_value() && window->IsQueueFamilySupportPresent(instance, *gpu, i)) {
            presentFamily.emplace(i);
        }

        if (graphicsFamily.has_value() && presentFamily.has_value() && computeFamily.has_value()) {
            break;
        }
    }

    // Search for dedicated compute queue family
    for (std::uint32_t i = 0; i < queueFamilies.size(); ++i) {
        if (!(queueFamilies[i].queueFlags & vk::QueueFlagBits::eGraphics) &&
            queueFamilies[i].queueFlags & vk::QueueFlagBits::eCompute
        ) {
            computeFamily.emplace(i);
            break;
        }
    }

    return { *graphicsFamily, *presentFamily, *computeFamily };
};

std::vector<vk::DeviceQueueCreateInfo> GetQueueCreateInfos(const QueueFamilyIndices& indices) {
    std::array uniqueQueueFamilies { indices.m_graphicsFamily, indices.m_presentFamily, indices.m_computeFamily };
    std::ranges::sort(uniqueQueueFamilies);
    const std::size_t uniqueCount = std::distance(uniqueQueueFamilies.begin(), std::ranges::unique(uniqueQueueFamilies).begin());

    static float queuePriority = 0.5f;
    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    queueCreateInfos.reserve(uniqueCount);
    for (std::size_t i = 0; i < uniqueCount; ++i) {
        vk::DeviceQueueCreateInfo queueCreateInfo;
        queueCreateInfo.queueFamilyIndex = uniqueQueueFamilies.at(i);
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    return queueCreateInfos;
}
}

namespace Vulkan {
CDevice::CDevice(
    const IWindow* window,
    const CInstance& instance,
    const CPhysicalDevice& physicalDevice
) {
    // Setup features
    const std::vector<const char*> enabledExtensions = SetupExtensions(physicalDevice);

    auto features = physicalDevice->getFeatures2<vk::PhysicalDeviceFeatures2,
        vk::PhysicalDeviceVulkan11Features,
        vk::PhysicalDeviceVulkan13Features,
        vk::PhysicalDeviceVulkan14Features,
        vk::PhysicalDeviceMaintenance5Features
    >();

    void* pNext = nullptr;

    const auto& availableFeatures = features.get<vk::PhysicalDeviceFeatures2>();
    const auto& availableFeatures11 = features.get<vk::PhysicalDeviceVulkan11Features>();
    const auto& availableFeatures13 = features.get<vk::PhysicalDeviceVulkan13Features>();
    vk::PhysicalDeviceFeatures2 features2 {};
    vk::PhysicalDeviceVulkan11Features features11 {};
    vk::PhysicalDeviceVulkan13Features features13 {};
    vk::PhysicalDeviceVulkan14Features features14 {};
    vk::PhysicalDeviceMaintenance5Features maintenance5Features {};

    RequireFeature(availableFeatures.features, features2.features, &vk::PhysicalDeviceFeatures::samplerAnisotropy, "samplerAnisotropy");
    RequireFeature(availableFeatures11, features11, &vk::PhysicalDeviceVulkan11Features::shaderDrawParameters, "shaderDrawParameters");
    RequireFeature(availableFeatures13, features13, &vk::PhysicalDeviceVulkan13Features::synchronization2, "synchronization2");
    RequireFeature(availableFeatures13, features13, &vk::PhysicalDeviceVulkan13Features::dynamicRendering, "dynamicRendering");
    RequireFeature(availableFeatures13, features13, &vk::PhysicalDeviceVulkan13Features::maintenance4, "maintenance4");

    if (instance.ApiVersion() == vk::ApiVersion14) {
        const auto& availableFeatures14 = features.get<vk::PhysicalDeviceVulkan14Features>();
        OptionalFeature(availableFeatures14, features14, &vk::PhysicalDeviceVulkan14Features::maintenance5, m_caps.m_maintenance5);
        Utils::LinkPNextChain(pNext, &features14);
    } else {
        if (m_enabledExtensions.contains(vk::KHRMaintenance5ExtensionName)) {
            const auto& availableMaintenance5Features = features.get<vk::PhysicalDeviceMaintenance5Features>();
            OptionalFeature(availableMaintenance5Features, maintenance5Features, &vk::PhysicalDeviceMaintenance5Features::maintenance5, m_caps.m_maintenance5);
            Utils::LinkPNextChain(pNext, &maintenance5Features);
        }
    }

    Utils::LinkPNextChain(pNext, &features2);
    Utils::LinkPNextChain(pNext, &features11);
    Utils::LinkPNextChain(pNext, &features13);

    // Setup queue create infos
    const QueueFamilyIndices queueFamilyIndices = GetQueueFamilies(window, **instance, physicalDevice);
    const std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos = GetQueueCreateInfos(queueFamilyIndices);


    vk::DeviceCreateInfo deviceCreateInfo;
    deviceCreateInfo.pNext = pNext;
    deviceCreateInfo.queueCreateInfoCount = static_cast<std::uint32_t>(queueCreateInfos.size());
    deviceCreateInfo.pQueueCreateInfos = !queueCreateInfos.empty() ? queueCreateInfos.data() : nullptr;
    deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(enabledExtensions.size());
    deviceCreateInfo.ppEnabledExtensionNames = !enabledExtensions.empty() ? enabledExtensions.data() : nullptr;

    m_handle = vk::raii::Device { *physicalDevice, deviceCreateInfo };
    VULKAN_HPP_DEFAULT_DISPATCHER.init(*m_handle);

    m_graphicsQueue = CQueue { m_handle, queueFamilyIndices.m_graphicsFamily, 0 };
    m_presentQueue = CQueue { m_handle, queueFamilyIndices.m_presentFamily, 0 };
    m_computeQueue = CQueue { m_handle, queueFamilyIndices.m_computeFamily, 0 };
    m_transferQueue = CQueue { m_handle, queueFamilyIndices.m_transferFamily, 0 };
}

std::vector<const char*> CDevice::SetupExtensions(const CPhysicalDevice& gpu) {
    const std::unordered_map<std::string, bool> requestedExtensions = RequestExtensions();

    // Find these extensions
    for (const auto& extension : gpu->enumerateDeviceExtensionProperties()) {
        if (requestedExtensions.contains(extension.extensionName)) {
            m_enabledExtensions.insert(extension.extensionName);
        }
    }

    std::vector<std::string_view> missingExtensions {};
    for (const auto& [name, required] : requestedExtensions) {
        if (!m_enabledExtensions.contains(name) && required) {
            missingExtensions.emplace_back(name);
        }
    }

    // Some required extensions are missing...
    if (!missingExtensions.empty()) {
        throw std::runtime_error(
            fmt::format("System doesn't have required device extensions:\n    {}", fmt::join(missingExtensions, "\n    "))
        );
    }

    // Raw extension names
    std::vector<const char*> enabledExtensions {};
    enabledExtensions.reserve(m_enabledExtensions.size());
    for (const auto& ext : m_enabledExtensions) {
        enabledExtensions.push_back(ext.c_str());
    }

    return enabledExtensions;
}
}
