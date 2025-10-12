#include "host_buffer.hpp"

namespace Vulkan {
CHostBuffer::CHostBuffer(
    const CContext& context,
    const vk::DeviceSize size,
    const vk::BufferUsageFlags& usage
) : m_context(&context) {
    vk::BufferCreateInfo bufferInfo {};
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = vk::SharingMode::eExclusive;

    vma::AllocationCreateInfo allocInfo {};
    allocInfo.usage = vma::MemoryUsage::eAuto;
    allocInfo.flags = vma::AllocationCreateFlagBits::eHostAccessSequentialWrite;

    std::tie(m_handle, m_allocation) = m_context->GetAllocator().createBufferUnique(bufferInfo, allocInfo);
}

auto CHostBuffer::Clear() -> void {
    m_handle.reset();
    m_allocation.reset();
    m_context = nullptr;
}

auto CHostBuffer::Map() -> CMemoryMapping {
    return { m_context->GetAllocator(), *m_allocation };
}
}
