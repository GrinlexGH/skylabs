#include <skylabs/core/render/vulkan/memory/device_buffer.hpp>

namespace Vulkan {
CDeviceBuffer::CDeviceBuffer(
    const CContext& context,
    const vk::DeviceSize size,
    const vk::BufferUsageFlags& usage
) {
    vk::BufferCreateInfo bufferInfo {};
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = vk::SharingMode::eExclusive;

    vma::AllocationCreateInfo allocInfo {};
    allocInfo.usage = vma::MemoryUsage::eAuto;

    m_handle = vma::raii::Buffer { *context.Allocator(), bufferInfo, allocInfo };
}
}
