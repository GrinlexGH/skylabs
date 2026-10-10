#include "skylabs/engine/render/vulkan/context/allocator.hpp"

namespace sk::render::vulkan {
Allocator::Allocator(const vk::raii::Instance& instance, const Device& device) {
    vma::AllocatorCreateFlags flags = vma::AllocatorCreateFlagBits::eKhrMaintenance4 |
                                      vma::AllocatorCreateFlagBits::eBufferDeviceAddress;

    if (device.Caps().maintenance5) {
        flags |= vma::AllocatorCreateFlagBits::eKhrMaintenance5;
    }

    if (device.Caps().memoryPriority) {
        flags |= vma::AllocatorCreateFlagBits::eExtMemoryPriority;
    }

    vma::AllocatorCreateInfo allocatorCreateInfo;
    allocatorCreateInfo.flags = flags;
    allocatorCreateInfo.vulkanApiVersion = device.Caps().apiVersion;
    allocatorCreateInfo.physicalDevice = *device.GetPhysicalDevice();

    m_handle = vma::raii::Allocator { instance, *device, allocatorCreateInfo };
}
}
