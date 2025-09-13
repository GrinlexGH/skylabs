#include "image.hpp"

namespace {
auto FindMemoryType(
    const vk::raii::PhysicalDevice& physicalDevice,
    const std::uint32_t typeFilter,
    vk::MemoryPropertyFlags properties
) -> std::uint32_t {
    const static vk::PhysicalDeviceMemoryProperties memProperties = physicalDevice.getMemoryProperties();

    for (std::uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("Failed to find suitable memory type!");
}
}

namespace Vulkan {
CImage::CImage(std::nullptr_t) {}

CImage::CImage(
    const CContext* context,
    vk::Extent3D extent,
    vk::Format format,
    vk::ImageTiling tiling,
    vk::ImageUsageFlags usage,
    vk::ImageAspectFlags imageAspectFlags,
    vk::MemoryPropertyFlags memoryProperties
) {
    const vk::raii::Device& deviceHandle = context->GetDevice().GetHandle();

    vk::ImageCreateInfo imageInfo {};
    imageInfo.imageType = vk::ImageType::e2D;
    imageInfo.extent = extent;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = vk::ImageLayout::eUndefined;
    imageInfo.usage = usage;
    imageInfo.sharingMode = vk::SharingMode::eExclusive;
    imageInfo.samples = vk::SampleCountFlagBits::e1;
    imageInfo.flags = {};

    m_handle = deviceHandle.createImage(imageInfo);

    const vk::MemoryRequirements memRequirements = m_handle.getMemoryRequirements();

    vk::MemoryAllocateInfo allocInfo {};
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(
        context->GetPhysicalDevice()->GetHandle(),
        memRequirements.memoryTypeBits,
        memoryProperties
    );

    m_memory = deviceHandle.allocateMemory(allocInfo);
    m_handle.bindMemory(m_memory, 0);

    vk::ImageViewCreateInfo imageViewInfo {};
    imageViewInfo.image = m_handle;
    imageViewInfo.viewType = vk::ImageViewType::e2D;
    imageViewInfo.format = format;
    imageViewInfo.subresourceRange.aspectMask = imageAspectFlags;
    imageViewInfo.subresourceRange.baseMipLevel = 0;
    imageViewInfo.subresourceRange.levelCount = 1;
    imageViewInfo.subresourceRange.baseArrayLayer = 0;
    imageViewInfo.subresourceRange.layerCount = 1;

    m_view = deviceHandle.createImageView(imageViewInfo);
}
}
