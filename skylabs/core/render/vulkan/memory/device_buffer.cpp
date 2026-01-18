#include <skylabs/core/render/vulkan/memory/device_buffer.hpp>

namespace Vulkan {
CDeviceBuffer::CDeviceBuffer(
    const CContext& context,
    const vk::DeviceSize size,
    const vk::BufferUsageFlags usage
) : m_size(size), m_usage(usage) {
    vk::BufferCreateInfo bufferInfo {};
    bufferInfo.size = m_size;
    bufferInfo.usage = m_usage;
    bufferInfo.sharingMode = vk::SharingMode::eExclusive;

    vma::AllocationCreateInfo allocInfo {};
    allocInfo.usage = vma::MemoryUsage::eAuto;

    m_handle = vma::raii::Buffer { *context.Allocator(), bufferInfo, allocInfo };
}
}
