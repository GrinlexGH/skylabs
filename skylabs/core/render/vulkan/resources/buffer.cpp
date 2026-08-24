#include <skylabs/core/render/vulkan/resources/buffer.hpp>

namespace Vulkan {
CBuffer::CBuffer(
    const vma::raii::Allocator& allocator,
    const vk::DeviceSize size,
    const vk::BufferUsageFlags& usage,
    const MemoryLocation location
) : m_size(size), m_usage(usage) {
    vk::BufferCreateInfo bufferInfo {};
    bufferInfo.size = m_size;
    bufferInfo.usage = m_usage;
    bufferInfo.sharingMode = vk::SharingMode::eExclusive;

    vma::AllocationCreateInfo allocCreateInfo {};
    switch (location) {
        case MemoryLocation::eDeviceOnly:
            allocCreateInfo.usage = vma::MemoryUsage::eAutoPreferDevice;
            break;
        case MemoryLocation::eHostVisible:
            allocCreateInfo.usage = vma::MemoryUsage::eAutoPreferHost;
            allocCreateInfo.flags = vma::AllocationCreateFlagBits::eHostAccessSequentialWrite |
                              vma::AllocationCreateFlagBits::eMapped;
            break;
    }

    vma::AllocationInfo allocationInfo;
    m_handle = vma::raii::Buffer {
        allocator,
        bufferInfo,
        allocCreateInfo,
        vk::Optional { allocationInfo }
    };

    if (allocCreateInfo.flags & vma::AllocationCreateFlagBits::eMapped) {
        m_data = allocationInfo.pMappedData;
    } else {
        m_data = nullptr;
    }

    const vma::VirtualBlockCreateInfo virtualBlockInfo { size };
    m_memoryBlock = vma::raii::VirtualBlock { virtualBlockInfo };
}
}
