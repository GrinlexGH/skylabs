#include "device.hpp"

#include "console.hpp"
#include <set>
#include <sstream>
#include <optional>

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

        throw std::runtime_error(error.str());
    }

    m_graphics = *graphicsQueueIndex;
    m_present = *presentQueueIndex;
    m_transfer = *transferQueueIndex;
    m_compute = *computeQueueIndex;
}
}

namespace Vulkan {
CDevice::CDevice(
    const CInstance& instance,
    const CPhysicalDevice& physicalDevice,
    const IVulkanWindow* const window
) {
    Msg("Selected device: {}", std::string_view{physicalDevice.GetProperties().deviceName});

    //====================
    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;

    CQueueFamilies queueIndices(instance.GetHandle(), physicalDevice, window);

    std::set uniqueQueueFamilies {
        queueIndices.m_graphics,
        queueIndices.m_present,
        queueIndices.m_transfer
    };

    float queuePriority = 0.5f;
    for (std::uint32_t queueFamily : uniqueQueueFamilies) {
        vk::DeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    //====================
    std::vector<const char*> enabledExtensions;

    vk::PhysicalDeviceFeatures requestedDeviceFeatures;

    vk::DeviceCreateInfo deviceCreateInfo;
    deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
    deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    deviceCreateInfo.pEnabledFeatures = &requestedDeviceFeatures;
    deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(enabledExtensions.size());
    deviceCreateInfo.ppEnabledExtensionNames = enabledExtensions.data();

    m_handle = physicalDevice.GetHandle().createDevice(deviceCreateInfo);

    m_graphicsQueue = std::make_unique<CQueue>(m_handle, queueIndices.m_graphics);
    m_presentQueue = std::make_unique<CQueue>(m_handle, queueIndices.m_present);
    m_transferQueue = std::make_unique<CQueue>(m_handle, queueIndices.m_transfer);

    //Create(requiredExtensions);

    //m_queues.Init(m_handle, m_queueFamilies);

    //m_swapchain.Init(m_physicalDevice.GetHandle(), m_handle, window->GetSurface());

    //m_allocator.Create(instance, m_physicalDevice.GetHandle(), m_handle);
}

CDevice::~CDevice() {
    if (m_handle) {
        m_handle.destroy();
    }
}
}
