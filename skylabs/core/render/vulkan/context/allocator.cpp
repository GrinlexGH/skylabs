#include <skylabs/core/render/vulkan/context/allocator.hpp>

namespace Vulkan {
CAllocator::CAllocator(
    const vk::raii::Instance& instance,
    const vk::raii::PhysicalDevice& physicalDevice,
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
    allocatorCreateInfo.vulkanApiVersion = device.ApiVersion();
    allocatorCreateInfo.physicalDevice = physicalDevice;

    m_handle = vma::raii::Allocator { instance, *device, allocatorCreateInfo };
}
}
