#include <skylabs/core/render/vulkan/context/device.hpp>
#include <skylabs/core/render/vulkan/context/extensions.hpp>
#include <skylabs/public/logging.hpp>

namespace Vulkan {
CDevice::CDevice(
    const IWindow* window,
    const vk::Instance instance,
    const CPhysicalDevice& gpu,
    const std::uint32_t apiVersion,
    std::span<CRequestedFeature> requestedFeatures
) : m_apiVersion(apiVersion) {
    // Setup queue create infos
    auto [graphicsFamily, presentFamily, computeFamily] = GetQueueFamilies(window, instance, gpu);

    // Unique array
    std::array uniqueQueueFamilies { graphicsFamily, presentFamily, computeFamily };
    std::ranges::sort(uniqueQueueFamilies);
    const std::size_t uniqueCount = std::distance(uniqueQueueFamilies.begin(), std::ranges::unique(uniqueQueueFamilies).begin());

    static constexpr float queuePriority = 0.5f;
    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    queueCreateInfos.reserve(uniqueCount);
    for (std::size_t i = 0; i < uniqueCount; ++i) {
        vk::DeviceQueueCreateInfo queueCreateInfo;
        queueCreateInfo.queueFamilyIndex = uniqueQueueFamilies.at(i);
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    // Setup features
    DeviceFeatures finalFeatures;

    if (apiVersion < vk::ApiVersion14) {
        finalFeatures.unlink<vk::PhysicalDeviceVulkan14Features>();

        if (apiVersion < vk::ApiVersion13) {
            finalFeatures.unlink<vk::PhysicalDeviceVulkan13Features>();

            if (apiVersion < vk::ApiVersion12) {
                finalFeatures.unlink<vk::PhysicalDeviceVulkan12Features>();

                if (apiVersion < vk::ApiVersion11) {
                    finalFeatures.unlink<vk::PhysicalDeviceVulkan11Features>();
                }
            }
        }
    }

    std::vector<const char*> enabledExtensions {};
    for (const auto& [enable, requirement] : requestedFeatures) {
        if (!enable({ .m_apiVersion = apiVersion, .m_gpu = gpu, .m_features = finalFeatures, .m_deviceExtensions = enabledExtensions })) {
            if (requirement == ::Utils::Requirement::eRequired) {
                throw std::runtime_error("System can't enable required feature! See logs");
            }
            Log::Debug("Can't enable optional feature. See logs");
        }
    }

    // Remove duplicates
    std::ranges::sort(enabledExtensions);
    enabledExtensions.erase(enabledExtensions.end(), std::ranges::unique(enabledExtensions).end());

    for (const auto name : enabledExtensions) {
        m_activeExtensions.emplace_back(name);
    }

    vk::DeviceCreateInfo deviceCreateInfo;
    deviceCreateInfo.queueCreateInfoCount = static_cast<std::uint32_t>(queueCreateInfos.size());
    deviceCreateInfo.pQueueCreateInfos = !queueCreateInfos.empty() ? queueCreateInfos.data() : nullptr;
    deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(enabledExtensions.size());
    deviceCreateInfo.ppEnabledExtensionNames = !enabledExtensions.empty() ? enabledExtensions.data() : nullptr;
    deviceCreateInfo.pNext = (m_enabledFeatures = finalFeatures).get<vk::PhysicalDeviceFeatures2KHR>();

    auto* current = static_cast<const vk::BaseInStructure*>(deviceCreateInfo.pNext);
    while (current) {
        Log::Debug("{}", vk::to_string(current->sType));
        current = current->pNext;
    }

    m_handle = vk::raii::Device { *gpu, deviceCreateInfo };
    VULKAN_HPP_DEFAULT_DISPATCHER.init(*m_handle);

    m_graphicsQueue = CQueue { vk::raii::Queue { m_handle, graphicsFamily, 0 }, graphicsFamily };
    m_presentQueue = CQueue { vk::raii::Queue { m_handle, presentFamily, 0 }, presentFamily };
    m_computeQueue = CQueue { vk::raii::Queue { m_handle, computeFamily, 0 }, computeFamily };
}

CDevice::CQueueFamilyIndices CDevice::GetQueueFamilies(const IWindow* window, const vk::Instance instance, const CPhysicalDevice& gpu) {
    std::optional<std::uint32_t> graphicsFamily;
    std::optional<std::uint32_t> presentFamily;
    std::optional<std::uint32_t> computeFamily;
    const auto& queueFamilies = gpu.QueueFamilies();

    for (uint32_t i = 0; i < queueFamilies.size(); ++i) {
        if (!graphicsFamily.has_value() && queueFamilies[i].queueFamilyProperties.queueFlags & vk::QueueFlagBits::eGraphics) {
            graphicsFamily.emplace(i);
        }

        // Graphics queue guarantees compute queue (but not on the same physical device...)
        if (!computeFamily.has_value() && queueFamilies[i].queueFamilyProperties.queueFlags & vk::QueueFlagBits::eCompute) {
            computeFamily.emplace(i);
        }

        if (!presentFamily.has_value() && window->IsQueueFamilySupportPresent(instance, *gpu, i)) {
            presentFamily.emplace(i);
        }

        if (graphicsFamily.has_value() && presentFamily.has_value() && computeFamily.has_value()) {
            break;
        }
    }

    return { .m_graphicsFamily = *graphicsFamily, .m_presentFamily = *presentFamily, .m_computeFamily = *computeFamily };
};
}
