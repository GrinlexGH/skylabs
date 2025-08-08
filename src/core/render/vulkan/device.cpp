#include "device.hpp"

#include <set>
#include <sstream>
#include <optional>

#include "physical_device.hpp"

namespace {
struct CQueueFamilies
{
    std::uint32_t m_graphics;
    std::uint32_t m_present;
    std::uint32_t m_transfer;
    std::uint32_t m_compute;
};

CQueueFamilies GetQueueFamilies(
    const vk::Instance& instance,
    const Vulkan::CPhysicalDevice& physicalDevice,
    const Vulkan::IWindow* window
) {
    const std::vector<vk::QueueFamilyProperties>& queueFamilies = physicalDevice.GetQueueFamilies();
    const vk::PhysicalDevice physicalDeviceHandle = physicalDevice.GetHandle();

    std::optional<std::uint32_t> graphicsQueueIndex;
    std::optional<std::uint32_t> presentQueueIndex;
    std::optional<std::uint32_t> transferQueueIndex;
    std::optional<std::uint32_t> computeQueueIndex;

    bool allQueuesFound = false;

    for (std::uint32_t i = 0; const auto& queue : queueFamilies) {
        if (queue.queueCount == 0) {
            ++i;
            continue;
        }

        if (queue.queueFlags & vk::QueueFlagBits::eGraphics) {
            graphicsQueueIndex.emplace(i);
        }

        if (window->CheckQueuePresentSupport(instance, physicalDeviceHandle, i)) {
            presentQueueIndex.emplace(i);
        }

        if (queue.queueFlags & vk::QueueFlagBits::eTransfer) {
            transferQueueIndex.emplace(i);
        }

        if (queue.queueFlags & vk::QueueFlagBits::eCompute) {
            computeQueueIndex.emplace(i);
        }

        if (graphicsQueueIndex.has_value() &&
            presentQueueIndex.has_value() &&
            transferQueueIndex.has_value() &&
            computeQueueIndex.has_value()
        ) {
            allQueuesFound = true;
            break;
        }

        ++i;
    }

    // Compute and graphics queue can implicitly accept transfer commands
    if (!transferQueueIndex.has_value()) {
        transferQueueIndex = computeQueueIndex;
    }

    if (!allQueuesFound) {
        std::vector<const char*> missingQueues;
        missingQueues.reserve(4);

        if (!graphicsQueueIndex.has_value()) {
            missingQueues.emplace_back("graphics");
        }

        if (!presentQueueIndex.has_value()) {
            missingQueues.emplace_back("present");
        }

        if (!transferQueueIndex.has_value()) {
            missingQueues.emplace_back("transfer");
        }

        if (!computeQueueIndex.has_value()) {
            missingQueues.emplace_back("compute");
        }

        if (!missingQueues.empty()) {
            std::stringstream error;
            error << "System doesn't have required vulkan queue families: [";

            for (std::size_t i = 0; i < missingQueues.size(); ++i) {
                error << missingQueues[i];
                if (i < missingQueues.size() - 1) {
                    error << " | ";
                }
            }
            error << "]!";

            throw std::runtime_error(error.str());
        }
    }

    return {
        .m_graphics = *graphicsQueueIndex,
        .m_present = *presentQueueIndex,
        .m_transfer = *transferQueueIndex,
        .m_compute = *computeQueueIndex
    };
}

bool EnableExtension(
    const std::vector<vk::ExtensionProperties>& availableExtensions,
    const char* name,
    std::vector<const char*>& enabledExtensions
) {
    if (HasExtension(availableExtensions, name)) {
        if (!HasExtension(enabledExtensions, name)) {
            enabledExtensions.emplace_back(name);
        }
    } else {
        return false;
    }

    return true;
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
        graphicsQueueFamily,
        presentQueueFamily,
        transferQueueFamily,
        computeQueueFamily
    ] = GetQueueFamilies(instance.GetHandle(), physicalDevice, window);

    std::set uniqueQueueFamilies {
        graphicsQueueFamily,
        presentQueueFamily,
        transferQueueFamily,
        computeQueueFamily
    };

    const float queuePriority = 0.5f;
    for (std::uint32_t queueFamily : uniqueQueueFamilies) {
        vk::DeviceQueueCreateInfo queueCreateInfo;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    //====================
    std::vector<const char*> enabledExtensions;
    enabledExtensions.reserve(extensions.size());
    std::vector<const char*> missingExtensions;
    missingExtensions.reserve(extensions.size());

    for (const auto& [name, required] : extensions) {
        if (!EnableExtension(physicalDevice.GetExtensions(), name, enabledExtensions) && required) {
            missingExtensions.push_back(name);
        }
    }

    if (!missingExtensions.empty()) {
        std::ostringstream error;
        error << "System doesn't have required device extensions:\n";
        for (const auto name : missingExtensions) {
            error << '\t' << name << '\n';
        }
        throw std::runtime_error(error.str());
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
    createInfo.pEnabledFeatures = physicalDevice.GetExtensionFeaturePNext() ? nullptr : &physicalDevice.GetRequiredFeatures();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(enabledExtensions.size());
    createInfo.ppEnabledExtensionNames = enabledExtensions.data();
    createInfo.pNext = pNext;

    m_handle = physicalDevice.GetHandle().createDevice(createInfo);

    m_graphicsQueue = { .m_handle = m_handle.getQueue(graphicsQueueFamily, 0), .m_familyIndex = graphicsQueueFamily };
    m_presentQueue = { .m_handle = m_handle.getQueue(presentQueueFamily, 0), .m_familyIndex = presentQueueFamily };
    m_transferQueue = { .m_handle = m_handle.getQueue(transferQueueFamily, 0), .m_familyIndex = transferQueueFamily };
    m_computeQueue = { .m_handle = m_handle.getQueue(computeQueueFamily, 0), .m_familyIndex = computeQueueFamily };
}
}
