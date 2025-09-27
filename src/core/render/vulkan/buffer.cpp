#include "buffer.hpp"

namespace Vulkan {
CBuffer::CBuffer(
    const CContext* context,
    const vk::DeviceSize size,
    const vk::BufferUsageFlags& usage,
    const vk::MemoryPropertyFlags& memoryProperties
) : m_memoryProperties(memoryProperties), m_context(context) {
    vk::BufferCreateInfo bufferInfo {};
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = vk::SharingMode::eExclusive;

    vma::AllocationCreateInfo allocInfo {};
    allocInfo.usage = vma::MemoryUsage::eAuto;
    allocInfo.requiredFlags = m_memoryProperties;
    if (m_memoryProperties & vk::MemoryPropertyFlagBits::eHostVisible) {
        allocInfo.flags |= vma::AllocationCreateFlagBits::eHostAccessSequentialWrite;
    }

    std::tie(m_handle, m_allocation) = m_context->GetAllocator().GetHandle().createBufferUnique(bufferInfo, allocInfo);
}

auto CBuffer::Clear() -> void {
    m_handle.reset();
    m_allocation.reset();
    m_memoryProperties = {};
    m_context = nullptr;
}

auto CBuffer::CopyFromHost(const void* hostData, const std::size_t size) -> void {
    if (!(m_memoryProperties & vk::MemoryPropertyFlagBits::eHostVisible)) {
        throw std::runtime_error("Cannot map memory that is not host visible!"); // TODO: maybe assert?
    }

    const vma::Allocator& allocator = *m_context->GetAllocator();
    void* deviceData = allocator.mapMemory(*m_allocation);
    std::memcpy(deviceData, hostData, size);
    allocator.unmapMemory(*m_allocation);
}

CBuffer::~CBuffer() {
    m_memoryProperties = {};
    m_context = nullptr;
}
}
