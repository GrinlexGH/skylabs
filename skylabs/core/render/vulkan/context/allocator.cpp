#include <skylabs/core/render/vulkan/context/allocator.hpp>

namespace Vulkan {
CAllocator::CAllocator(const vk::raii::Instance& instance, const CDevice& device) {
    vma::AllocatorCreateFlags flags = vma::AllocatorCreateFlagBits::eKhrMaintenance4;

    if (device.Caps().maintenance5) {
        flags |= vma::AllocatorCreateFlagBits::eKhrMaintenance5;
    }

    vma::AllocatorCreateInfo allocatorCreateInfo;
    allocatorCreateInfo.flags = flags;
    allocatorCreateInfo.vulkanApiVersion = device.PhysicalDevice().ApiVersion();
    allocatorCreateInfo.physicalDevice = *device.PhysicalDevice();

    m_handle = vma::raii::Allocator { instance, *device, allocatorCreateInfo };
}
}
