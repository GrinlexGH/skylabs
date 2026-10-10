#include "skylabs/engine/render/vulkan/buffer.hpp"

namespace sk::render::vulkan {
Buffer::Buffer(const vma::raii::Allocator& allocator, const Device& device, const vk::DeviceSize size,
               const vk::BufferUsageFlags2& usage, const MemoryLocation location)
    : m_size(size), m_usage(usage) {
    vk::StructureChain<vk::BufferCreateInfo, vk::BufferUsageFlags2CreateInfo> chain;

    auto& bufferInfo = chain.get<vk::BufferCreateInfo>();
    bufferInfo.size = size;
    bufferInfo.sharingMode = vk::SharingMode::eExclusive;

    if (device.Caps().maintenance5 || device.Caps().extendedFlags) {
        chain.get<vk::BufferUsageFlags2CreateInfo>().usage = usage;
    } else {
        chain.unlink<vk::BufferUsageFlags2CreateInfo>();
        const auto raw = static_cast<VkBufferUsageFlags2>(usage);
        assert(raw >> 32u == 0 && "Usage requires VK_KHR_maintenance5");
        bufferInfo.usage = vk::BufferUsageFlags(static_cast<VkBufferUsageFlags>(raw));
    }

    vma::AllocationCreateInfo allocCreateInfo { };
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
    m_handle =
        vma::raii::Buffer { allocator, bufferInfo, allocCreateInfo, vk::Optional { allocationInfo } };

    if (allocCreateInfo.flags & vma::AllocationCreateFlagBits::eMapped) {
        m_data = allocationInfo.pMappedData;
    } else {
        m_data = nullptr;
    }

    if (usage & vk::BufferUsageFlagBits2::eShaderDeviceAddress) {
        m_address = device->getBufferAddress({ m_handle });
    }

    const vma::VirtualBlockCreateInfo virtualBlockInfo { size };
    m_memoryBlock = vma::raii::VirtualBlock { virtualBlockInfo };
}

vk::DeviceAddress Buffer::Address() const {
    if (m_usage & vk::BufferUsageFlagBits2::eShaderDeviceAddress) {
        return m_address;
    }

    throw std::runtime_error("Buffer was created without eShaderDeviceAddress");
}
}
