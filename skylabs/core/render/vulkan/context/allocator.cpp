#include "skylabs/core/render/vulkan/context/allocator.hpp"

namespace sk::render::vulkan {
Allocator::Allocator(const vk::raii::Instance& instance, const Device& device) {
    vma::AllocatorCreateFlags flags = vma::AllocatorCreateFlagBits::eKhrMaintenance4;

    if (device.Caps().maintenance5) {
        flags |= vma::AllocatorCreateFlagBits::eKhrMaintenance5;
    }

    vma::AllocatorCreateInfo allocatorCreateInfo;
    allocatorCreateInfo.flags = flags;
    allocatorCreateInfo.vulkanApiVersion = device.GetPhysicalDevice().ApiVersion();
    allocatorCreateInfo.physicalDevice = *device.GetPhysicalDevice();

    m_handle = vma::raii::Allocator { instance, *device, allocatorCreateInfo };
}
}
