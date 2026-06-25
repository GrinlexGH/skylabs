module skylabs.vulkan.context;
import :allocator;

namespace Vulkan {
CAllocator::CAllocator(
    const CInstance& instance,
    const vk::raii::PhysicalDevice& physicalDevice,
    const CDevice& device
) {
    vma::AllocatorCreateFlags flags = vma::AllocatorCreateFlagBits::eKhrMaintenance4;

    if (device.Caps().m_maintenance5) {
        flags |= vma::AllocatorCreateFlagBits::eKhrMaintenance5;
    }

    vma::AllocatorCreateInfo allocatorCreateInfo;
    allocatorCreateInfo.flags = flags;
    allocatorCreateInfo.vulkanApiVersion = instance.ApiVersion();
    allocatorCreateInfo.physicalDevice = physicalDevice;

    m_handle = vma::raii::Allocator { *instance, *device, allocatorCreateInfo };
}
}
