#include <skylabs/core/render/vulkan/memory/image.hpp>

#include <frozen/map.h>

namespace {
struct TransitionRule
{
    vk::AccessFlags2KHR m_srcAccessMask;
    vk::AccessFlags2KHR m_dstAccessMask;
    vk::PipelineStageFlags2KHR m_srcStageMask;
    vk::PipelineStageFlags2KHR m_dstStageMask;
};

constexpr frozen::map<const std::pair<vk::ImageLayout, vk::ImageLayout>, TransitionRule, 2> g_transitionRules = {
    {
        { vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal },
        {
            .m_srcAccessMask = {},
            .m_dstAccessMask = vk::AccessFlagBits2KHR::eTransferWrite,
            .m_srcStageMask = vk::PipelineStageFlagBits2KHR::eTopOfPipe,
            .m_dstStageMask = vk::PipelineStageFlagBits2KHR::eTransfer
        }
    },
    {
        { vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal },
        {
            .m_srcAccessMask = vk::AccessFlagBits2KHR::eTransferWrite,
            .m_dstAccessMask = vk::AccessFlagBits2KHR::eShaderRead,
            .m_srcStageMask = vk::PipelineStageFlagBits2KHR::eTransfer,
            .m_dstStageMask = vk::PipelineStageFlagBits2KHR::eFragmentShader
        }
    }
};
}

namespace Vulkan {
CImage::CImage(
    const CContext& context,
    const vk::Extent3D& extent,
    const vk::Format format,
    const vk::ImageTiling tiling,
    const vk::ImageUsageFlags& usage,
    const vk::ImageAspectFlags& imageAspectFlags,
    const vk::MemoryPropertyFlags& memoryProperties,
    const std::uint32_t mipLevels,
    const vk::SampleCountFlagBits sampleCount
) {
    vk::ImageCreateInfo imageInfo {};
    imageInfo.imageType = vk::ImageType::e2D;
    imageInfo.extent = m_extent = extent;
    imageInfo.mipLevels = m_mipLevels = mipLevels;
    imageInfo.arrayLayers = 1;
    imageInfo.format = m_format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = vk::ImageLayout::eUndefined;
    imageInfo.usage = usage;
    imageInfo.sharingMode = vk::SharingMode::eExclusive;
    imageInfo.samples = m_sampleCount = sampleCount;
    imageInfo.flags = {};

    vma::AllocationCreateInfo allocInfo {};
    allocInfo.usage = vma::MemoryUsage::eAuto;
    allocInfo.requiredFlags = memoryProperties;

    std::tie(m_allocation, m_handle) = context.GetAllocator().createImageUnique(imageInfo, allocInfo);

    vk::ImageViewCreateInfo imageViewInfo {};
    imageViewInfo.image = *m_handle;
    imageViewInfo.viewType = vk::ImageViewType::e2D;
    imageViewInfo.format = format;
    imageViewInfo.subresourceRange.aspectMask = imageAspectFlags;
    imageViewInfo.subresourceRange.baseMipLevel = 0;
    imageViewInfo.subresourceRange.levelCount = m_mipLevels;
    imageViewInfo.subresourceRange.baseArrayLayer = 0;
    imageViewInfo.subresourceRange.layerCount = 1;

    m_view = vk::raii::ImageView { *context.GetDevice(), imageViewInfo };
}

void CImage::Clear() {
    m_handle.reset();
    m_allocation.reset();
    m_view.clear();
    m_layout = vk::ImageLayout::eUndefined;
    m_format = vk::Format::eUndefined;
}

void CImage::TransitionLayout(const vk::raii::CommandBuffer& commandBuffer, vk::ImageLayout newLayout) {
    const auto it = g_transitionRules.find(std::make_pair(m_layout, newLayout));
    if (it == g_transitionRules.end()) {
        throw std::invalid_argument("Unsupported layout transition!");
    }

    const auto& [
        srcAccessMask,
        dstAccessMask,
        srcStageMask,
        dstStageMask
    ] = it->second;

    vk::ImageMemoryBarrier2KHR barrier {};
    barrier.srcStageMask = srcStageMask;
    barrier.srcAccessMask = srcAccessMask;
    barrier.dstStageMask = dstStageMask;
    barrier.dstAccessMask = dstAccessMask;
    barrier.oldLayout = m_layout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
    barrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;
    barrier.image = *m_handle;
    barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = m_mipLevels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    vk::DependencyInfoKHR dependencyInfo {};
    dependencyInfo.imageMemoryBarrierCount = 1;
    dependencyInfo.pImageMemoryBarriers = &barrier;

    commandBuffer.pipelineBarrier2(dependencyInfo);

    m_layout = newLayout;
}

void CImage::CopyBufferToImage(
    const vk::raii::CommandBuffer& commandBuffer,
    const vk::Buffer& buffer,
    const vk::Extent3D& extent
) {
    vk::BufferImageCopy region;
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;

    region.imageOffset = vk::Offset3D {0, 0, 0};
    region.imageExtent = extent;

    commandBuffer.copyBufferToImage(buffer, *m_handle, vk::ImageLayout::eTransferDstOptimal, region);
}

CImage::~CImage() {
    m_layout = vk::ImageLayout::eUndefined;
}
}
