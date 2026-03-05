#include <skylabs/core/render/vulkan/context/device.hpp>
#include <skylabs/core/render/vulkan/context/extensions.hpp>

#include <fmt/ranges.h>

struct CQueueFamilyIndices
{
    std::uint32_t m_graphicsFamily = 0;
    std::uint32_t m_presentFamily = 0;
    std::uint32_t m_computeFamily = 0;
};

namespace {
std::unordered_map<std::string, bool> RequestExtensions() {
    std::unordered_map<std::string, bool> requestedExtensions;
    auto requestExtension = [&](const char* name, bool required = false) { requestedExtensions.try_emplace(name, required); };

    requestExtension(vk::KHRSwapchainExtensionName, true);

    requestExtension(vk::EXTMemoryBudgetExtensionName, false);
    requestExtension(vk::AMDDeviceCoherentMemoryExtensionName, false);
    requestExtension(vk::EXTMemoryPriorityExtensionName, false);
    requestExtension(vk::KHRMaintenance5ExtensionName, false);
#ifdef PLATFORM_WINDOWS
    requestExtension(vk::KHRExternalMemoryWin32ExtensionName, false);
#endif

    return requestedExtensions;
}

CQueueFamilyIndices GetQueueFamilies(const Vulkan::IWindow* window, const vk::Instance instance, const Vulkan::CPhysicalDevice& gpu) {
    std::optional<std::uint32_t> graphicsFamily;
    std::optional<std::uint32_t> presentFamily;
    std::optional<std::uint32_t> computeFamily;
    const std::vector<vk::QueueFamilyProperties>& queueFamilies = gpu->getQueueFamilyProperties();

    for (std::uint32_t i = 0; i < queueFamilies.size(); ++i) {
        if (!graphicsFamily.has_value() && queueFamilies[i].queueFlags & vk::QueueFlagBits::eGraphics) {
            graphicsFamily.emplace(i);
        }

        // Graphics queue guarantees compute queue
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

    return { .m_graphicsFamily = *graphicsFamily, .m_presentFamily = *presentFamily, .m_computeFamily = *computeFamily };
};

std::vector<vk::DeviceQueueCreateInfo> GetQueueCreateInfos(const CQueueFamilyIndices& indices) {
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
    const CProfile profile,
    const IWindow* window,
    const CInstance& instance,
    const CPhysicalDevice& physicalDevice
) {
    // Setup queue create infos
    const CQueueFamilyIndices queueFamilyIndices = GetQueueFamilies(window, **instance, physicalDevice);
    const std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos = GetQueueCreateInfos(queueFamilyIndices);

    // Setup features
    const std::vector<const char*> enabledExtensions = SetupExtensions(profile, physicalDevice);

    void* pNext = nullptr;
    vk::PhysicalDeviceVulkan11Features features11;

    // Enable shader draw parameters on roadmap 2022 profile
    if (profile.GetCurrentProfile() == CProfile::Profile::eRoadmap2022) {
        features11.shaderDrawParameters = vk::True;
        Utils::AppendToPNextChain(pNext, &features11);
    }

    vk::DeviceCreateInfo deviceCreateInfo;
    deviceCreateInfo.pNext = pNext;
    deviceCreateInfo.queueCreateInfoCount = static_cast<std::uint32_t>(queueCreateInfos.size());
    deviceCreateInfo.pQueueCreateInfos = !queueCreateInfos.empty() ? queueCreateInfos.data() : nullptr;
    deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(enabledExtensions.size());
    deviceCreateInfo.ppEnabledExtensionNames = !enabledExtensions.empty() ? enabledExtensions.data() : nullptr;

    m_handle = vk::raii::Device { *physicalDevice, profile.CreateDevice(**physicalDevice, deviceCreateInfo) };
    VULKAN_HPP_DEFAULT_DISPATCHER.init(*m_handle);

    m_graphicsQueue = CQueue { m_handle, queueFamilyIndices.m_graphicsFamily, 0 };
    m_presentQueue = CQueue { m_handle, queueFamilyIndices.m_presentFamily, 0 };
    m_computeQueue = CQueue { m_handle, queueFamilyIndices.m_computeFamily, 0 };
}

std::vector<const char*> CDevice::SetupExtensions(const CProfile profile, const CPhysicalDevice& gpu) {
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

    // Add extensions from profile
    for (const auto& [name, _] : profile.GetDeviceExtensions()) {
        m_enabledExtensions.insert(name);
    }

    return enabledExtensions;
}
}
