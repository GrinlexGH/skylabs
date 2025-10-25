#include <skylabs/core/render/vulkan/context/allocator.hpp>

namespace Vulkan {
CAllocator::CAllocator(
    const CInstance& instance,
    const vk::PhysicalDevice& physicalDevice,
    const CDevice& device
) {
    constexpr std::array extensionAndFlagMap = {
        std::make_pair(vk::KHRDedicatedAllocationExtensionName, vma::AllocatorCreateFlagBits::eKhrDedicatedAllocation),
        std::make_pair(vk::KHRBindMemory2ExtensionName, vma::AllocatorCreateFlagBits::eKhrBindMemory2),
        std::make_pair(vk::EXTMemoryBudgetExtensionName, vma::AllocatorCreateFlagBits::eExtMemoryBudget),
        std::make_pair(vk::AMDDeviceCoherentMemoryExtensionName, vma::AllocatorCreateFlagBits::eAmdDeviceCoherentMemory),
        std::make_pair(vk::KHRBufferDeviceAddressExtensionName, vma::AllocatorCreateFlagBits::eBufferDeviceAddress),
        std::make_pair(vk::EXTMemoryPriorityExtensionName, vma::AllocatorCreateFlagBits::eExtMemoryPriority),
        std::make_pair(vk::KHRMaintenance4ExtensionName, vma::AllocatorCreateFlagBits::eKhrMaintenance4),
        std::make_pair(vk::KHRMaintenance5ExtensionName, vma::AllocatorCreateFlagBits::eKhrMaintenance5),
    #ifdef PLATFORM_WINDOWS
        std::make_pair(vk::KHRExternalMemoryWin32ExtensionName, vma::AllocatorCreateFlagBits::eKhrExternalMemoryWin32),
    #endif
    };

    vma::AllocatorCreateFlags flags {};
    for (const auto& [ext, flag] : extensionAndFlagMap) {
        if (device.IsExtensionEnabled(ext)) {
            flags |= flag;
        }
    }

    vma::AllocatorCreateInfo allocatorCreateInfo;
    allocatorCreateInfo.flags = flags;
    allocatorCreateInfo.vulkanApiVersion = instance.GetApiVersion();
    allocatorCreateInfo.physicalDevice = physicalDevice;
    allocatorCreateInfo.device = *device;
    allocatorCreateInfo.instance = *instance;

    vma::VulkanFunctions vulkanFunctions = vma::functionsFromDispatcher((*instance).getDispatcher(), (*device).getDispatcher());

#ifdef VK_USE_PLATFORM_WIN32_KHR
    vulkanFunctions.vkGetMemoryWin32HandleKHR = (*device).getDispatcher()->vkGetMemoryWin32HandleKHR;
#endif

    allocatorCreateInfo.pVulkanFunctions = &vulkanFunctions;

    m_handle = vma::UniqueAllocator{ vma::createAllocator(allocatorCreateInfo) };
}
}
