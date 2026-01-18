#include <skylabs/core/render/vulkan/memory/image.hpp>

namespace {
vk::ImageMemoryBarrier2 GetBarrierData(const vk::ImageLayout oldLayout, const vk::ImageLayout newLayout) {
    auto getStageMask = [](const vk::ImageLayout layout) -> std::tuple<vk::PipelineStageFlags2, vk::AccessFlags2> {
        switch (layout) {
            case vk::ImageLayout::eUndefined:
                return {
                    vk::PipelineStageFlagBits2::eNone,
                    vk::AccessFlagBits2::eNone
                };

            case vk::ImageLayout::eColorAttachmentOptimal:
                return {
                    vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                    vk::AccessFlagBits2::eColorAttachmentWrite
                };

            case vk::ImageLayout::eDepthStencilAttachmentOptimal:
                return {
                    vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
                    vk::AccessFlagBits2::eDepthStencilAttachmentWrite
                };

            case vk::ImageLayout::eShaderReadOnlyOptimal:
                return {
                    vk::PipelineStageFlagBits2::eFragmentShader,
                    vk::AccessFlagBits2::eShaderRead
                };

            case vk::ImageLayout::eTransferDstOptimal:
                return {
                    vk::PipelineStageFlagBits2::eTransfer,
                    vk::AccessFlagBits2::eTransferWrite
                };

            case vk::ImageLayout::ePresentSrcKHR:
                return {
                    vk::PipelineStageFlagBits2::eNone,
                    vk::AccessFlagBits2::eNone
                };

            default:
                assert(false && "Unsupported layout transition!");
                return {
                    vk::PipelineStageFlagBits2::eAllCommands,
                    vk::AccessFlagBits2::eMemoryRead | vk::AccessFlagBits2::eMemoryWrite
                };
        }
    };

    vk::ImageMemoryBarrier2 barrier {};
    std::tie(barrier.srcStageMask, barrier.srcAccessMask) = getStageMask(oldLayout);
    std::tie(barrier.dstStageMask, barrier.dstAccessMask) = getStageMask(newLayout);

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
    const vk::raii::CommandBuffer& cmd,
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

    cmd.pipelineBarrier2(dependencyInfo);
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
