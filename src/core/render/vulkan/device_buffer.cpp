#include "device_buffer.hpp"

namespace Vulkan {
CDeviceBuffer::CDeviceBuffer(
    const CContext* context,
    const vk::DeviceSize size,
    const vk::BufferUsageFlags& usage
) : m_context(context) {
    vk::BufferCreateInfo bufferInfo {};
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = vk::SharingMode::eExclusive;

    vma::AllocationCreateInfo allocInfo {};
    allocInfo.usage = vma::MemoryUsage::eAuto;

    std::tie(m_handle, m_allocation) = m_context->GetAllocator().createBufferUnique(bufferInfo, allocInfo);
}

auto CDeviceBuffer::Clear() -> void {
    m_handle.reset();
    m_allocation.reset();
    m_context = nullptr;
}

CDeviceBuffer::~CDeviceBuffer() {
    m_context = nullptr;
}
}
