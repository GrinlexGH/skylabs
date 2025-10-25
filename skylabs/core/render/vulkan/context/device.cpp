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

auto ThrowIfMissing(
    const std::optional<std::uint32_t>& graphicsIndex,
    const std::optional<std::uint32_t>& presentIndex,
    const std::optional<std::uint32_t>& transferIndex,
    const std::optional<std::uint32_t>& computeIndex
) -> void {
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
        throw std::runtime_error("System doesn't have required Vulkan queue families: [" + error + "]!");
    }
}

auto GetQueueFamilies(
    const vk::Instance& instance,
    const Vulkan::CPhysicalDevice& physicalDevice,
    const Vulkan::IWindow* window
) -> CQueueFamilies {
    const std::vector<vk::QueueFamilyProperties>& queueFamilies = physicalDevice.GetQueueFamilies();
    const vk::PhysicalDevice physicalDeviceHandle = physicalDevice.GetHandle();

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

        if (window->IsQueueFamilyPresentSupport(instance, physicalDeviceHandle, i)) {
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

auto EnableExtension(
    const Vulkan::CPhysicalDevice& physicalDevice,
    const char* name,
    std::vector<const char*>& enabledExtensions
) -> bool {
    if (HasExtension(enabledExtensions, name)) {
        return true;
    }

    if (physicalDevice.IsExtensionSupported(name)) {
        enabledExtensions.emplace_back(name);
        return true;
    }

    return false;
}
}

namespace Vulkan {
CDevice::CDevice(
    const CInstance& instance,
    const CPhysicalDevice& physicalDevice,
    const IWindow* const window,
    const std::unordered_map<const char*, bool>& extensions
) {
    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;

    auto [
        graphicsFamily,
        presentFamily,
        transferFamily,
        computeFamily
    ] = GetQueueFamilies(instance.GetHandle(), physicalDevice, window);

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
    m_enabledExtensions.reserve(extensions.size());
    std::vector<const char*> missingExtensions;
    missingExtensions.reserve(extensions.size());

    for (const auto& [name, required] : extensions) {
        if (!EnableExtension(physicalDevice, name, m_enabledExtensions) && required) {
            missingExtensions.push_back(name);
        }
    }

    if (!missingExtensions.empty()) {
        std::string error;
        error.reserve(missingExtensions.size() * 20 + 50);
        error += "System doesn't have required device extensions:\n";
        for (const auto name : missingExtensions) {
            error += '\t';
            error += name;
            error += '\n';
        }
        throw std::runtime_error(error);
    }

    //====================
    void* pNext = nullptr;
    vk::PhysicalDeviceFeatures2 features;

    if (physicalDevice.GetExtensionFeaturePNext()) {
        features.pNext = physicalDevice.GetExtensionFeaturePNext();
        features.features = physicalDevice.GetRequiredFeatures();
        AppendToPNextChain(pNext, &features);
    }

    vk::DeviceCreateInfo createInfo;
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pEnabledFeatures = physicalDevice.GetExtensionFeaturePNext()
        ? nullptr
        : &physicalDevice.GetRequiredFeatures();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(m_enabledExtensions.size());
    createInfo.ppEnabledExtensionNames = m_enabledExtensions.data();
    createInfo.pNext = pNext;

    m_handle = physicalDevice.GetHandle().createDevice(createInfo);
    VULKAN_HPP_DEFAULT_DISPATCHER.init(*m_handle);

    m_graphicsQueue = { .m_handle = m_handle.getQueue(graphicsFamily, 0), .m_familyIndex = graphicsFamily };
    m_presentQueue = { .m_handle = m_handle.getQueue(presentFamily, 0), .m_familyIndex = presentFamily };
    m_transferQueue = { .m_handle = m_handle.getQueue(transferFamily, 0), .m_familyIndex = transferFamily };
    m_computeQueue = { .m_handle = m_handle.getQueue(computeFamily, 0), .m_familyIndex = computeFamily };
}
}
