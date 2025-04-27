#include "device.hpp"

#include <set>
#include <sstream>
#include <optional>

#include "physical_device.hpp"

namespace {
struct CQueueFamilies
{
    explicit CQueueFamilies(
        const vk::Instance& instance,
        const Vulkan::CPhysicalDevice& physicalDevice,
        const IVulkanWindow* window
    );

    std::uint32_t m_graphics;
    std::uint32_t m_present;
    std::uint32_t m_transfer;
    std::uint32_t m_compute;
};

CQueueFamilies::CQueueFamilies(
    const vk::Instance& instance,
    const Vulkan::CPhysicalDevice& physicalDevice,
    const IVulkanWindow* const window
) {
    const std::vector<vk::QueueFamilyProperties>& queueFamilies = physicalDevice.GetQueueFamilies();
    std::optional<std::uint32_t> graphicsQueueIndex;
    std::optional<std::uint32_t> presentQueueIndex;
    std::optional<std::uint32_t> transferQueueIndex;
    std::optional<std::uint32_t> computeQueueIndex;

    for (std::uint32_t i = 0; const auto& queue : queueFamilies) {
        if (queue.queueCount == 0) {
            ++i;
            continue;
        }

        if (queue.queueFlags & vk::QueueFlagBits::eGraphics) {
            graphicsQueueIndex = i;
        }

        if (window->CheckQueuePresentSupport(instance, physicalDevice.GetHandle(), i)) {
            presentQueueIndex = i;
        }

        if (queue.queueFlags & vk::QueueFlagBits::eTransfer) {
            transferQueueIndex = i;
        }

        if (queue.queueFlags & vk::QueueFlagBits::eCompute) {
            computeQueueIndex = i;
        }

        if (graphicsQueueIndex.has_value() &&
            presentQueueIndex.has_value() &&
            transferQueueIndex.has_value() &&
            computeQueueIndex.has_value()
        ) {
            break;
        }

        ++i;
    }

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

        for (std::size_t j = 0; j < missingQueues.size(); ++j) {
            error << missingQueues[j];
            if (j < missingQueues.size() - 1) {
                error << " | ";
            }
        }
        error << "]!";

        throw CRendererError(error.str());
    }

    m_graphics = *graphicsQueueIndex;   //-V1007
    m_present = *presentQueueIndex;     //-V1007
    m_transfer = *transferQueueIndex;   //-V1007
    m_compute = *computeQueueIndex;     //-V1007
}

bool EnableExtension(
    const std::vector<vk::ExtensionProperties>& availableExtensions,
    const char* name,
    std::vector<const char*>& enabledExtensions
) {
    if (HasExtension(availableExtensions, name)) {
        if (!HasExtension(enabledExtensions, name)) {
            enabledExtensions.push_back(name);
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
    const IVulkanWindow* const window,
    const std::unordered_map<const char*, bool>& extensions
) {
    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;

    CQueueFamilies queueIndices(instance.GetHandle(), physicalDevice, window);

    std::set uniqueQueueFamilies {
        queueIndices.m_graphics,
        queueIndices.m_present,
        queueIndices.m_transfer,
        queueIndices.m_compute
    };

    float queuePriority = 0.5f;
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
        error << "System doesn't have required instance extensions:\n";
        for (const auto name : missingExtensions) {
            error << '\t' << name << '\n';
        }
        throw CRendererError(error.str());
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

    m_graphicsQueue = std::make_unique<CQueue>(m_handle, queueIndices.m_graphics);
    m_presentQueue = std::make_unique<CQueue>(m_handle, queueIndices.m_present);
    m_transferQueue = std::make_unique<CQueue>(m_handle, queueIndices.m_transfer);
    m_computeQueue = std::make_unique<CQueue>(m_handle, queueIndices.m_compute);
}

CDevice::~CDevice() {
    if (m_handle) {
        m_handle.destroy();
    }
}
}
