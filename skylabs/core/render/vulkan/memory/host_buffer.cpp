#include <skylabs/core/render/vulkan/memory/host_buffer.hpp>

namespace Vulkan {
CHostBuffer::CHostBuffer(
    const CContext& context,
    const vk::DeviceSize size,
    const vk::BufferUsageFlags& usage
) : m_context(&context) {
    vk::BufferCreateInfo bufferInfo {};
    bufferInfo.size = m_size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = vk::SharingMode::eExclusive;

    vma::AllocationCreateInfo allocInfo {};
    allocInfo.usage = vma::MemoryUsage::eAuto;
    allocInfo.flags = vma::AllocationCreateFlagBits::eHostAccessSequentialWrite;

    m_handle = vma::raii::Buffer { *context.Allocator(), bufferInfo, allocInfo };
    m_data = m_handle.getAllocation().map();
}

CHostBuffer::~CHostBuffer() {
    if (*m_handle) {
        m_handle.getAllocation().unmap();
    }
}
}
