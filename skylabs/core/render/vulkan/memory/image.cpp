#include <skylabs/core/render/vulkan/memory/image.hpp>

namespace {
vk::ImageMemoryBarrier2 GetBarrierData(const vk::ImageLayout oldLayout, const vk::ImageLayout newLayout) {
    vk::ImageMemoryBarrier2 barrier {};
    barrier.srcStageMask = vk::PipelineStageFlagBits2::eAllCommands;
    barrier.srcAccessMask = vk::AccessFlagBits2::eMemoryWrite;
    barrier.dstStageMask = vk::PipelineStageFlagBits2::eAllCommands;
    barrier.dstAccessMask = vk::AccessFlagBits2::eMemoryRead | vk::AccessFlagBits2::eMemoryWrite;

    // 1. Undefined -> Color Attachment
    if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eColorAttachmentOptimal) {
        barrier.srcStageMask = vk::PipelineStageFlagBits2::eTopOfPipe;
        barrier.srcAccessMask = vk::AccessFlagBits2::eNone;
        barrier.dstStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
        barrier.dstAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite;
        return barrier;
    }

    // 2. Undefined -> Depth Attachment
    if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
        barrier.srcStageMask = vk::PipelineStageFlagBits2::eTopOfPipe;
        barrier.srcAccessMask = vk::AccessFlagBits2::eNone;
        barrier.dstStageMask = vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests;
        barrier.dstAccessMask = vk::AccessFlagBits2::eDepthStencilAttachmentWrite;
        return barrier;
    }

    // 3. Color Attachment -> Shader Read
    if (oldLayout == vk::ImageLayout::eColorAttachmentOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
        barrier.srcStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
        barrier.srcAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite;
        barrier.dstStageMask = vk::PipelineStageFlagBits2::eFragmentShader;
        barrier.dstAccessMask = vk::AccessFlagBits2::eShaderRead;
        return barrier;
    }

    // 4. Transfer -> Shader Read
    if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
        barrier.srcStageMask = vk::PipelineStageFlagBits2::eTransfer;
        barrier.srcAccessMask = vk::AccessFlagBits2::eTransferWrite;
        barrier.dstStageMask = vk::PipelineStageFlagBits2::eFragmentShader;
        barrier.dstAccessMask = vk::AccessFlagBits2::eShaderRead;
        return barrier;
    }

    // 5. Color Attachment -> Present
    if (oldLayout == vk::ImageLayout::eColorAttachmentOptimal && newLayout == vk::ImageLayout::ePresentSrcKHR) {
        barrier.srcStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
        barrier.srcAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite;
        barrier.dstStageMask = vk::PipelineStageFlagBits2::eBottomOfPipe;
        barrier.dstAccessMask = vk::AccessFlagBits2::eNone;
        return barrier;
    }

    assert(false && "Unsupported layout transition!");
    return barrier;
}
}

namespace Vulkan {
CImage::CImage(
    const CContext& context,
    const vk::Extent2D& extent,
    const vk::Format format,
    const vk::ImageTiling tiling,
    const vk::ImageUsageFlags usage,
    const vk::ImageAspectFlags imageAspectFlags,
    const std::uint32_t mipLevels,
    const vk::SampleCountFlagBits sampleCount
) : m_format(format),
    m_extent(extent),
    m_mipLevels(mipLevels),
    m_sampleCount(sampleCount),
    m_aspectFlags(imageAspectFlags)
{
    vk::ImageCreateInfo imageInfo {};
    imageInfo.imageType = vk::ImageType::e2D;
    imageInfo.format = m_format;
    imageInfo.extent = vk::Extent3D { extent, 1 };
    imageInfo.mipLevels = m_mipLevels;
    imageInfo.arrayLayers = 1;
    imageInfo.samples = m_sampleCount;
    imageInfo.tiling = tiling;
    imageInfo.usage = usage;
    imageInfo.sharingMode = vk::SharingMode::eExclusive;
    imageInfo.initialLayout = m_layout;

    vma::AllocationCreateInfo allocInfo {};
    allocInfo.usage = vma::MemoryUsage::eAuto;
    allocInfo.requiredFlags = vk::MemoryPropertyFlagBits::eDeviceLocal;

    m_handle = vma::raii::Image { *context.Allocator(), imageInfo, allocInfo };

    vk::ImageViewCreateInfo imageViewInfo {};
    imageViewInfo.image = *m_handle;
    imageViewInfo.viewType = vk::ImageViewType::e2D;
    imageViewInfo.format = m_format;
    imageViewInfo.subresourceRange.aspectMask = m_aspectFlags;
    imageViewInfo.subresourceRange.baseMipLevel = 0;
    imageViewInfo.subresourceRange.levelCount = m_mipLevels;
    imageViewInfo.subresourceRange.baseArrayLayer = 0;
    imageViewInfo.subresourceRange.layerCount = 1;

    m_view = vk::raii::ImageView { *context.Device(), imageViewInfo };
}

void CImage::Clear() {
    m_handle.clear();
    m_view.clear();
    m_format = vk::Format::eUndefined;
    m_extent = vk::Extent2D {};
    m_mipLevels = 1;
    m_sampleCount = vk::SampleCountFlagBits::e1;
    m_layout = vk::ImageLayout::eUndefined;
    m_aspectFlags = vk::ImageAspectFlagBits::eNone;
}

void CImage::CmdTransitionLayout(
    const vk::CommandBuffer commandBuffer,
    const vk::Image image,
    const vk::ImageLayout oldLayout,
    const vk::ImageLayout newLayout,
    const vk::ImageAspectFlags aspectMask,
    const std::uint32_t mipLevels
) {
    auto barrier = GetBarrierData(oldLayout, newLayout);

    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
    barrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;
    barrier.image = image;

    barrier.subresourceRange.aspectMask = aspectMask;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = mipLevels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    vk::DependencyInfo dependencyInfo {};
    dependencyInfo.imageMemoryBarrierCount = 1;
    dependencyInfo.pImageMemoryBarriers = &barrier;

    commandBuffer.pipelineBarrier2(dependencyInfo);
}

void CImage::TransitionLayout(const vk::raii::CommandBuffer& commandBuffer, const vk::ImageLayout newLayout) {
    CmdTransitionLayout(commandBuffer, *m_handle, m_layout, newLayout, m_aspectFlags, m_mipLevels);
    m_layout = newLayout;
}

void CImage::CopyBufferToImage(
    const vk::raii::CommandBuffer& commandBuffer,
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
