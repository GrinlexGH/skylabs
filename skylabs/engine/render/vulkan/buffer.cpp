#include "skylabs/engine/render/vulkan/buffer.hpp"

namespace sk::render::vulkan {
Buffer::Buffer(const Device& device, const vma::raii::Allocator& allocator, const vk::DeviceSize size,
               const MemoryLocation location, vk::BufferUsageFlags2 usage,
               const std::optional<vk::DeviceSize>& virtualAllocationSize)
    : m_size(size) {
    vk::StructureChain<vk::BufferCreateInfo, vk::BufferUsageFlags2CreateInfo> createInfo;

    vk::BufferCreateInfo& bufferCreateInfo = createInfo.get<vk::BufferCreateInfo>();
    bufferCreateInfo.size = size;
    bufferCreateInfo.sharingMode = vk::SharingMode::eExclusive;

    usage |= vk::BufferUsageFlagBits2::eShaderDeviceAddress;

    if (device.Caps().maintenance5 || device.Caps().extendedFlags) {
        createInfo.get<vk::BufferUsageFlags2CreateInfo>().usage = usage;
    } else {
        createInfo.unlink<vk::BufferUsageFlags2CreateInfo>();
        const VkBufferUsageFlags2 raw = static_cast<VkBufferUsageFlags2>(usage);
        assert(raw >> 32u == 0 && "Usage requires VK_KHR_maintenance5");
        bufferCreateInfo.usage = vk::BufferUsageFlags { static_cast<VkBufferUsageFlags>(raw) };
    }

    vma::AllocationCreateInfo allocationCreateInfo { };
    if (location == MemoryLocation::eDeviceOnly) {
        allocationCreateInfo.usage = vma::MemoryUsage::eAutoPreferDevice;
    } else {
        allocationCreateInfo.usage = vma::MemoryUsage::eAutoPreferHost;
        allocationCreateInfo.flags = vma::AllocationCreateFlagBits::eHostAccessSequentialWrite |
                                     vma::AllocationCreateFlagBits::eMapped;
    }

    vma::AllocationInfo allocationInfo;
    m_handle = vma::raii::Buffer { allocator, bufferCreateInfo, allocationCreateInfo,
                                   vk::Optional { allocationInfo } };

    m_data = allocationInfo.pMappedData;
    m_address = device->getBufferAddress({ *m_handle });

    vma::VirtualBlockCreateInfo virtualBlockInfo { };
    virtualBlockInfo.size = virtualAllocationSize.value_or(size);

    m_memoryBlock = vma::raii::VirtualBlock { virtualBlockInfo };
}
}
