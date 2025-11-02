#include <skylabs/core/render/vulkan/memory/image.hpp>

#include <map>

namespace {
struct TransitionRule
{
    vk::AccessFlags2KHR m_srcAccessMask;
    vk::AccessFlags2KHR m_dstAccessMask;
    vk::PipelineStageFlags2KHR m_srcStageMask;
    vk::PipelineStageFlags2KHR m_dstStageMask;
};

auto GetTransitionRules() -> auto& {
    static const std::map<std::pair<vk::ImageLayout, vk::ImageLayout>, TransitionRule> transitionRules = {
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

    return transitionRules;
}
}

namespace Vulkan {
CImage::CImage(
    const CContext& context,
    const vk::Extent3D& extent,
    const vk::Format format,
    const vk::ImageTiling tiling,
    const vk::ImageUsageFlags& usage,
    const vk::ImageAspectFlags& imageAspectFlags,
    const vk::MemoryPropertyFlags& memoryProperties
) {
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
    imageViewInfo.subresourceRange.levelCount = 1;
    imageViewInfo.subresourceRange.baseArrayLayer = 0;
    imageViewInfo.subresourceRange.layerCount = 1;

    m_view = vk::raii::ImageView { *context.GetDevice(), imageViewInfo };
}

auto CImage::Clear() -> void {
    m_handle.reset();
    m_allocation.reset();
    m_view.clear();
    m_layout = vk::ImageLayout::eUndefined;
}

auto CImage::TransitionLayout(const vk::raii::CommandBuffer& commandBuffer, vk::ImageLayout newLayout) -> void {
    const auto key = std::make_pair(m_layout, newLayout);
    const auto it = GetTransitionRules().find(key);
    if (it == GetTransitionRules().end()) {
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
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    vk::DependencyInfoKHR dependencyInfo {};
    dependencyInfo.imageMemoryBarrierCount = 1;
    dependencyInfo.pImageMemoryBarriers = &barrier;

    commandBuffer.pipelineBarrier2(dependencyInfo);

    m_layout = newLayout;
}

auto CImage::CopyBufferToImage(
    const vk::raii::CommandBuffer& commandBuffer,
    const vk::Buffer& buffer,
    const vk::Extent3D& extent
) -> void {
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
