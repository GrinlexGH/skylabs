#include <skylabs/core/render/vulkan/resources/image.hpp>

namespace Vulkan {
CImage::CImage(const CContext& context, const ImageCreateInfo options) {
    vk::ImageViewType viewType;

    vk::ImageCreateInfo imageInfo {};
    imageInfo.format = m_format = options.m_format;
    imageInfo.extent = m_extent = options.m_extent;

    if (imageInfo.extent.height == 1 && imageInfo.extent.depth == 1) {
        imageInfo.imageType = vk::ImageType::e1D;
        if (options.m_arrayLevels == 1)
            viewType = vk::ImageViewType::e1D;
        else
            viewType = vk::ImageViewType::e1DArray;
    } else if (imageInfo.extent.depth == 1) {
        imageInfo.imageType = vk::ImageType::e2D;
        if (options.m_arrayLevels == 1)
            viewType = vk::ImageViewType::e2D;
        else
            viewType = vk::ImageViewType::e2DArray;
    } else {
        assert(options.m_arrayLevels == 1);
        imageInfo.imageType = vk::ImageType::e3D;
        viewType = vk::ImageViewType::e3D;
    }

    imageInfo.mipLevels = m_mipLevels = options.m_mipLevels;
    imageInfo.arrayLayers = m_arrayLevels = options.m_arrayLevels;
    imageInfo.samples = m_sampleCount = options.m_sampleCount;
    imageInfo.tiling = vk::ImageTiling::eOptimal;
    imageInfo.usage = options.m_usageFlags;
    imageInfo.sharingMode = vk::SharingMode::eExclusive;
    imageInfo.queueFamilyIndexCount = 0;
    imageInfo.initialLayout = vk::ImageLayout::eUndefined;

    vma::AllocationCreateInfo allocInfo {};
    allocInfo.usage = vma::MemoryUsage::eAuto;
    allocInfo.requiredFlags = vk::MemoryPropertyFlagBits::eDeviceLocal;

    m_handle = vma::raii::Image { *context.Allocator(), imageInfo, allocInfo };

    if (m_format == vk::Format::eD32Sfloat || m_format == vk::Format::eD16Unorm) {
        m_aspectFlags = vk::ImageAspectFlagBits::eDepth;
    } else if (m_format == vk::Format::eD24UnormS8Uint || m_format == vk::Format::eD32SfloatS8Uint) {
        m_aspectFlags = vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
    } else {
        m_aspectFlags = vk::ImageAspectFlagBits::eColor;
    }

    vk::ImageViewCreateInfo imageViewInfo {};
    imageViewInfo.image = *m_handle;
    imageViewInfo.viewType = viewType;
    imageViewInfo.format = m_format;
    imageViewInfo.subresourceRange.aspectMask = m_aspectFlags;
    imageViewInfo.subresourceRange.baseMipLevel = 0;
    imageViewInfo.subresourceRange.levelCount = m_mipLevels;
    imageViewInfo.subresourceRange.baseArrayLayer = 0;
    imageViewInfo.subresourceRange.layerCount = m_arrayLevels;

    m_view = vk::raii::ImageView { *context.Device(), imageViewInfo };
}

void CImage::Clear() {
    m_handle.clear();
    m_view.clear();
    m_format = vk::Format::eUndefined;
    m_extent = vk::Extent3D {};
    m_mipLevels = 1;
    m_sampleCount = vk::SampleCountFlagBits::e1;
    m_aspectFlags = vk::ImageAspectFlagBits::eNone;
}

void CImage::CopyBufferToImage(
    const vk::CommandBuffer& commandBuffer,
    const vk::Buffer& buffer,
    const vk::Extent2D& extent
) const {
    vk::BufferImageCopy region;
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;

    region.imageOffset = vk::Offset3D {0, 0, 0};
    region.imageExtent = vk::Extent3D { extent, 1 };

    commandBuffer.copyBufferToImage(buffer, *m_handle, vk::ImageLayout::eTransferDstOptimal, region);
}
}
