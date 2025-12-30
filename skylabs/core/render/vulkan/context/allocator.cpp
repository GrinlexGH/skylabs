#include <skylabs/core/render/vulkan/context/allocator.hpp>

namespace Vulkan {
CAllocator::CAllocator(
    const CInstance& instance,
    const vk::PhysicalDevice& physicalDevice,
    const CDevice& device
) {
    constexpr std::array extensionAndFlagMap = {
        std::pair { vk::KHRDedicatedAllocationExtensionName, vma::AllocatorCreateFlagBits::eKhrDedicatedAllocation },
        std::pair { vk::KHRBindMemory2ExtensionName, vma::AllocatorCreateFlagBits::eKhrBindMemory2 },
        std::pair { vk::EXTMemoryBudgetExtensionName, vma::AllocatorCreateFlagBits::eExtMemoryBudget },
        std::pair { vk::AMDDeviceCoherentMemoryExtensionName, vma::AllocatorCreateFlagBits::eAmdDeviceCoherentMemory },
        std::pair { vk::KHRBufferDeviceAddressExtensionName, vma::AllocatorCreateFlagBits::eBufferDeviceAddress },
        std::pair { vk::EXTMemoryPriorityExtensionName, vma::AllocatorCreateFlagBits::eExtMemoryPriority },
        std::pair { vk::KHRMaintenance4ExtensionName, vma::AllocatorCreateFlagBits::eKhrMaintenance4 },
        std::pair { vk::KHRMaintenance5ExtensionName, vma::AllocatorCreateFlagBits::eKhrMaintenance5 },
    #ifdef PLATFORM_WINDOWS
        std::pair { vk::KHRExternalMemoryWin32ExtensionName, vma::AllocatorCreateFlagBits::eKhrExternalMemoryWin32 },
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

    vma::VulkanFunctions vulkanFunctions = {
        (*instance).getDispatcher()->vkGetInstanceProcAddr,
        (*instance).getDispatcher()->vkGetDeviceProcAddr,
        (*instance).getDispatcher()->vkGetPhysicalDeviceProperties,
        (*instance).getDispatcher()->vkGetPhysicalDeviceMemoryProperties,
        (*device).getDispatcher()->vkAllocateMemory,
        (*device).getDispatcher()->vkFreeMemory,
        (*device).getDispatcher()->vkMapMemory,
        (*device).getDispatcher()->vkUnmapMemory,
        (*device).getDispatcher()->vkFlushMappedMemoryRanges,
        (*device).getDispatcher()->vkInvalidateMappedMemoryRanges,
        (*device).getDispatcher()->vkBindBufferMemory,
        (*device).getDispatcher()->vkBindImageMemory,
        (*device).getDispatcher()->vkGetBufferMemoryRequirements,
        (*device).getDispatcher()->vkGetImageMemoryRequirements,
        (*device).getDispatcher()->vkCreateBuffer,
        (*device).getDispatcher()->vkDestroyBuffer,
        (*device).getDispatcher()->vkCreateImage,
        (*device).getDispatcher()->vkDestroyImage,
        (*device).getDispatcher()->vkCmdCopyBuffer,
        (*device).getDispatcher()->vkGetBufferMemoryRequirements2KHR
            ? (*device).getDispatcher()->vkGetBufferMemoryRequirements2KHR
            : (*device).getDispatcher()->vkGetBufferMemoryRequirements2,
        (*device).getDispatcher()->vkGetImageMemoryRequirements2KHR
            ? (*device).getDispatcher()->vkGetImageMemoryRequirements2KHR
            : (*device).getDispatcher()->vkGetImageMemoryRequirements2,
        (*device).getDispatcher()->vkBindBufferMemory2KHR ? (*device).getDispatcher()->vkBindBufferMemory2KHR
                                       : (*device).getDispatcher()->vkBindBufferMemory2,
        (*device).getDispatcher()->vkBindImageMemory2KHR ? (*device).getDispatcher()->vkBindImageMemory2KHR
                                      : (*device).getDispatcher()->vkBindImageMemory2,
        (*instance).getDispatcher()->vkGetPhysicalDeviceMemoryProperties2KHR
            ? (*instance).getDispatcher()->vkGetPhysicalDeviceMemoryProperties2KHR
            : (*instance).getDispatcher()->vkGetPhysicalDeviceMemoryProperties2,
        (*device).getDispatcher()->vkGetDeviceBufferMemoryRequirementsKHR
            ? (*device).getDispatcher()->vkGetDeviceBufferMemoryRequirementsKHR
            : (*device).getDispatcher()->vkGetDeviceBufferMemoryRequirements,
        (*device).getDispatcher()->vkGetDeviceImageMemoryRequirementsKHR
            ? (*device).getDispatcher()->vkGetDeviceImageMemoryRequirementsKHR
            : (*device).getDispatcher()->vkGetDeviceImageMemoryRequirements,
    };

#ifdef VK_USE_PLATFORM_WIN32_KHR
    vulkanFunctions.vkGetMemoryWin32HandleKHR = (*device).getDispatcher()->vkGetMemoryWin32HandleKHR;
#endif

    allocatorCreateInfo.pVulkanFunctions = &vulkanFunctions;

    m_handle = vma::createAllocatorUnique(allocatorCreateInfo);
}
}
