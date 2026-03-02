#include <skylabs/core/render/vulkan/context/allocator.hpp>

namespace Vulkan {
CAllocator::CAllocator(
    const CProfile profile,
    const vk::raii::Instance& instance,
    const vk::raii::PhysicalDevice& physicalDevice,
    const CDevice& device
) {
    constexpr std::array extensionAndFlagMap = {
        std::pair { vk::EXTMemoryBudgetExtensionName, vma::AllocatorCreateFlagBits::eExtMemoryBudget },
        std::pair { vk::AMDDeviceCoherentMemoryExtensionName, vma::AllocatorCreateFlagBits::eAmdDeviceCoherentMemory },
        std::pair { vk::EXTMemoryPriorityExtensionName, vma::AllocatorCreateFlagBits::eExtMemoryPriority },
        std::pair { vk::KHRMaintenance5ExtensionName, vma::AllocatorCreateFlagBits::eKhrMaintenance5 },
#ifdef PLATFORM_WINDOWS
        std::pair { vk::KHRExternalMemoryWin32ExtensionName, vma::AllocatorCreateFlagBits::eKhrExternalMemoryWin32 },
#endif
    };

    // Available in roadmap
    vma::AllocatorCreateFlags flags =
        vma::AllocatorCreateFlagBits::eKhrDedicatedAllocation
        | vma::AllocatorCreateFlagBits::eKhrBindMemory2
        | vma::AllocatorCreateFlagBits::eBufferDeviceAddress
        | vma::AllocatorCreateFlagBits::eKhrMaintenance4;

    for (const auto& [ext, flag] : extensionAndFlagMap) {
        if (device.IsExtensionEnabled(ext)) {
            flags |= flag;
        }
    }

    vma::AllocatorCreateInfo allocatorCreateInfo;
    allocatorCreateInfo.flags = flags;
    allocatorCreateInfo.vulkanApiVersion = profile.GetAPIVersion();
    allocatorCreateInfo.physicalDevice = physicalDevice;

    m_handle = vma::raii::Allocator { instance, *device, allocatorCreateInfo };
}
}
