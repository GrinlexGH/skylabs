#include "skylabs/engine/render/vulkan/buffer.hpp"

namespace sk::render::vulkan {
Buffer::Buffer(const vma::raii::Allocator& allocator, const Device& device, const vk::DeviceSize size,
               const MemoryLocation location, const vk::BufferUsageFlags2& usage,
               const std::optional<vk::DeviceSize> virtualAllocationSize)
    : m_size(size) {
    vk::StructureChain<vk::BufferCreateInfo, vk::BufferUsageFlags2CreateInfo> chain;

    auto& bufferInfo = chain.get<vk::BufferCreateInfo>();
    bufferInfo.size = size;
    bufferInfo.sharingMode = vk::SharingMode::eExclusive;

    m_usage = usage | vk::BufferUsageFlagBits2::eShaderDeviceAddress;

    if (device.Caps().maintenance5 || device.Caps().extendedFlags) {
        chain.get<vk::BufferUsageFlags2CreateInfo>().usage = m_usage;
    } else {
        chain.unlink<vk::BufferUsageFlags2CreateInfo>();
        const auto raw = static_cast<VkBufferUsageFlags2>(m_usage);
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

    m_address = device->getBufferAddress({ m_handle });

    const vma::VirtualBlockCreateInfo virtualBlockInfo { virtualAllocationSize.value_or(size) };
    m_memoryBlock = vma::raii::VirtualBlock { virtualBlockInfo };
}
}
