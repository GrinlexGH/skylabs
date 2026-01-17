#include <skylabs/core/render/vulkan/memory/image.hpp>

#include <frozen/map.h>

namespace {
vk::ImageMemoryBarrier2 GetBarrierData(const vk::ImageLayout oldLayout, const vk::ImageLayout newLayout) {
    vk::ImageMemoryBarrier2 barrier {};
    barrier.srcStageMask = vk::PipelineStageFlagBits2::eAllCommands;
    barrier.srcAccessMask = vk::AccessFlagBits2::eMemoryWrite; // Грубая, но безопасная заглушка по умолчанию
    barrier.dstStageMask = vk::PipelineStageFlagBits2::eAllCommands;
    barrier.dstAccessMask = vk::AccessFlagBits2::eMemoryRead | vk::AccessFlagBits2::eMemoryWrite;

    // 1. Undefined -> Color Attachment (Начало рендера)
    if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eColorAttachmentOptimal) {
        barrier.srcStageMask = vk::PipelineStageFlagBits2::eTopOfPipe;
        barrier.srcAccessMask = vk::AccessFlagBits2::eNone;
        barrier.dstStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
        barrier.dstAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite;
        return barrier;
    }

    // 2. Undefined -> Depth Attachment (Начало рендера глубины)
    if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
        barrier.srcStageMask = vk::PipelineStageFlagBits2::eTopOfPipe;
        barrier.srcAccessMask = vk::AccessFlagBits2::eNone;
        barrier.dstStageMask = vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests;
        barrier.dstAccessMask = vk::AccessFlagBits2::eDepthStencilAttachmentWrite;
        return barrier;
    }

    // 3. Color Attachment -> Shader Read (Использование результата рендера как текстуры)
    if (oldLayout == vk::ImageLayout::eColorAttachmentOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
        barrier.srcStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
        barrier.srcAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite;
        barrier.dstStageMask = vk::PipelineStageFlagBits2::eFragmentShader;
        barrier.dstAccessMask = vk::AccessFlagBits2::eShaderRead;
        return barrier;
    }

    // 4. Transfer -> Shader Read (Загрузка текстур, старое правило)
    if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
        barrier.srcStageMask = vk::PipelineStageFlagBits2::eTransfer;
        barrier.srcAccessMask = vk::AccessFlagBits2::eTransferWrite;
        barrier.dstStageMask = vk::PipelineStageFlagBits2::eFragmentShader;
        barrier.dstAccessMask = vk::AccessFlagBits2::eShaderRead;
        return barrier;
    }

    // 5. Color Attachment -> Present (Для Swapchain перед показом на экран)
    if (oldLayout == vk::ImageLayout::eColorAttachmentOptimal && newLayout == vk::ImageLayout::ePresentSrcKHR) {
        barrier.srcStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
        barrier.srcAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite;
        barrier.dstStageMask = vk::PipelineStageFlagBits2::eBottomOfPipe;
        barrier.dstAccessMask = vk::AccessFlagBits2::eNone;
        return barrier;
    }

    return barrier;
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

    m_handle = vma::raii::Image { *context.Allocator(), imageInfo, allocInfo };

    vk::ImageViewCreateInfo imageViewInfo {};
    imageViewInfo.image = *m_handle;
    imageViewInfo.viewType = vk::ImageViewType::e2D;
    imageViewInfo.format = format;
    imageViewInfo.subresourceRange.aspectMask = m_aspectFlags = imageAspectFlags;
    imageViewInfo.subresourceRange.baseMipLevel = 0;
    imageViewInfo.subresourceRange.levelCount = m_mipLevels;
    imageViewInfo.subresourceRange.baseArrayLayer = 0;
    imageViewInfo.subresourceRange.layerCount = 1;

    m_view = vk::raii::ImageView { *context.Device(), imageViewInfo };
}

void CImage::Clear() {
    m_handle.clear();
    m_view.clear();
    m_layout = vk::ImageLayout::eUndefined;
    m_format = vk::Format::eUndefined;
}

void CImage::CmdTransitionLayout(
    const vk::CommandBuffer commandBuffer,
    const vk::Image image,
    const vk::ImageLayout oldLayout,
    const vk::ImageLayout newLayout,
    const vk::ImageAspectFlags aspectMask,
    const std::uint32_t mipLevels,
    const std::uint32_t layerCount
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
    barrier.subresourceRange.layerCount = layerCount;

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
    const vk::Extent3D& extent
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
    region.imageExtent = extent;

    commandBuffer.copyBufferToImage(buffer, *m_handle, vk::ImageLayout::eTransferDstOptimal, region);
}

CImage::~CImage() {
    m_layout = vk::ImageLayout::eUndefined;
    m_format = vk::Format::eUndefined;
    m_extent = vk::Extent3D {};
    m_mipLevels = 1;
    m_sampleCount = vk::SampleCountFlagBits::e1;
}
}
