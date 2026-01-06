#include <skylabs/core/render/vulkan/context/device.hpp>

#include <skylabs/core/render/vulkan/context/physical_device.hpp>
#include <skylabs/core/render/vulkan/window.hpp>

#include <array>
#include <optional>
#include <set>

namespace {
struct CQueueFamilies
{
    std::uint32_t m_graphics;
    std::uint32_t m_present;
    std::uint32_t m_transfer;
    std::uint32_t m_compute;
};

void ThrowIfMissing(
    const std::optional<std::uint32_t>& graphicsIndex,
    const std::optional<std::uint32_t>& presentIndex,
    const std::optional<std::uint32_t>& transferIndex,
    const std::optional<std::uint32_t>& computeIndex
) {
    const std::array<std::pair<const char*, bool>, 4> checks = {{
        { "graphics", graphicsIndex.has_value() },
        { "present", presentIndex.has_value() },
        { "transfer", transferIndex.has_value() },
        { "compute", computeIndex.has_value() }
    }};

    std::string error;
    error.reserve(checks.size() * 10);
    bool first = true;

    for (const auto& [name, hasValue] : checks) {
        if (!hasValue) {
            if (!first) {
                error += " | ";
            }
            error += name;
            first = false;
        }
    }

    if (!first) {
        throw std::runtime_error { "System doesn't have required Vulkan queue families: [" + error + "]!" };
    }
}

CQueueFamilies GetQueueFamilies(
    const vk::Instance& instance,
    const Vulkan::CPhysicalDevice& physicalDevice,
    const Vulkan::IWindow* window
) {
    const std::vector<vk::QueueFamilyProperties>& queueFamilies = physicalDevice.QueueFamilies();
    const vk::PhysicalDevice physicalDeviceHandle = physicalDevice.Handle();

    std::optional<std::uint32_t> graphicsIndex;
    std::optional<std::uint32_t> presentIndex;
    std::optional<std::uint32_t> transferIndex;
    std::optional<std::uint32_t> computeIndex;

    bool allQueuesFound = false;

    for (std::uint32_t i = 0; const auto& queue : queueFamilies) {
        if (queue.queueCount == 0) {
            ++i;
            continue;
        }

        if (queue.queueFlags & vk::QueueFlagBits::eGraphics) {
            graphicsIndex.emplace(i);
        }

        if (window->IsQueueFamilySupportPresent(instance, physicalDeviceHandle, i)) {
            presentIndex.emplace(i);
        }

        if (queue.queueFlags & vk::QueueFlagBits::eTransfer) {
            transferIndex.emplace(i);
        }

        if (queue.queueFlags & vk::QueueFlagBits::eCompute) {
            computeIndex.emplace(i);
        }

        if (graphicsIndex.has_value() &&
            presentIndex.has_value() &&
            transferIndex.has_value() &&
            computeIndex.has_value()
        ) {
            allQueuesFound = true;
            break;
        }

        ++i;
    }

    // Compute and graphics queue can implicitly accept transfer commands
    if (!transferIndex.has_value()) {
        transferIndex = computeIndex.has_value() ? computeIndex : graphicsIndex;
    }

    if (!allQueuesFound) {
        ThrowIfMissing(graphicsIndex, presentIndex, transferIndex, computeIndex);
    }

    return {
        .m_graphics = *graphicsIndex,
        .m_present = *presentIndex,
        .m_transfer = *transferIndex,
        .m_compute = *computeIndex
    };
}
}

namespace Vulkan {
CDevice::CDevice(
    const CInstance& instance,
    const CPhysicalDevice& physicalDevice,
    const IWindow* const window,
    const std::span<Utils::CRequestedExtension> extensions
) {
    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;

    auto [
        graphicsFamily,
        presentFamily,
        transferFamily,
        computeFamily
    ] = GetQueueFamilies(*instance, physicalDevice, window);

    const std::set uniqueQueueFamilies { graphicsFamily, presentFamily, transferFamily, computeFamily };

    queueCreateInfos.reserve(uniqueQueueFamilies.size());
    constexpr float queuePriority = 0.5f;
    for (const std::uint32_t queueFamily : uniqueQueueFamilies) {
        vk::DeviceQueueCreateInfo queueCreateInfo;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    //====================
    std::vector<const char*> enabledExtensions = EnableExtensions(extensions, physicalDevice, instance.ApiVersion());

    //====================
    void* pNext = nullptr;
    vk::PhysicalDeviceFeatures2 features;

    if (physicalDevice.ExtensionFeaturePNext()) {
        features.pNext = physicalDevice.ExtensionFeaturePNext();
        features.features = physicalDevice.RequiredFeatures();
        Utils::AppendToPNextChain(pNext, &features);
    }

    vk::DeviceCreateInfo createInfo;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = physicalDevice.ExtensionFeaturePNext()
        ? nullptr
        : &physicalDevice.RequiredFeatures();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(enabledExtensions.size());
    createInfo.ppEnabledExtensionNames = !enabledExtensions.empty() ? enabledExtensions.data() : nullptr;
    createInfo.pNext = pNext;

    m_handle = vk::raii::Device { *physicalDevice, createInfo };
    VULKAN_HPP_DEFAULT_DISPATCHER.init(*m_handle);

    m_graphicsQueue = { .m_handle = vk::raii::Queue { m_handle, graphicsFamily, 0 }, .m_familyIndex = graphicsFamily };
    m_presentQueue = { .m_handle = vk::raii::Queue { m_handle, presentFamily, 0 }, .m_familyIndex = presentFamily };
    m_transferQueue = { .m_handle = vk::raii::Queue { m_handle, transferFamily, 0 }, .m_familyIndex = transferFamily };
    m_computeQueue = { .m_handle = vk::raii::Queue { m_handle, computeFamily, 0 }, .m_familyIndex = computeFamily };
}

std::vector<const char*> CDevice::EnableExtensions(
    const std::span<Utils::CRequestedExtension> requestedExtensions,
    const CPhysicalDevice& physicalDevice,
    const std::uint32_t apiVersion
) {
    std::vector<const char*> enabledExtensions;
    enabledExtensions.reserve(requestedExtensions.size());

    std::vector<const char*> missingExtensions;
    missingExtensions.reserve(4);

    m_enabledExtensions.reserve(requestedExtensions.size());

    for (const auto& extension : requestedExtensions) {
        const std::string_view name = extension.Name();
        const Utils::ExtensionRequirement requirement = extension.Requirement();

        if (apiVersion >= extension.PromotedVersion())
            continue;

        if (!m_enabledExtensions.contains(name)) {
            if (physicalDevice.IsExtensionSupported(name)) {
                enabledExtensions.emplace_back(name.data());
                m_enabledExtensions.emplace(name);
            }
        } else if (requirement == Utils::ExtensionRequirement::Required) {
            missingExtensions.push_back(name.data());
        }
    }

    if (!missingExtensions.empty()) {
        std::string error;
        error.reserve((missingExtensions.size() * 20) + 50);
        error += "System doesn't have required device extensions:\n";
        for (const auto name : missingExtensions) {
            error += '\t';
            error += name;
            error += '\n';
        }
        throw std::runtime_error { error };
    }

    return enabledExtensions;
}
}
