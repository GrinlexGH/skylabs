#include <skylabs/core/render/vulkan/command_buffer.hpp>

#include <frozen/map.h>

namespace {
constexpr frozen::map<Vulkan::Usage, std::tuple<vk::PipelineStageFlags2, vk::AccessFlags2, vk::ImageLayout>, 7> g_usageState {
    { Vulkan::Usage::eNone, { vk::PipelineStageFlagBits2::eNone, vk::AccessFlagBits2::eNone, vk::ImageLayout::eUndefined } },
    { Vulkan::Usage::ePresent, { vk::PipelineStageFlagBits2::eNone, vk::AccessFlagBits2::eNone, vk::ImageLayout::ePresentSrcKHR } },
    { Vulkan::Usage::eColorAttachment, { vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::AccessFlagBits2::eColorAttachmentWrite, vk::ImageLayout::eColorAttachmentOptimal } },
    { Vulkan::Usage::eDepthWrite, {
        vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
        vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
        vk::ImageLayout::eDepthStencilAttachmentOptimal
    } },
    { Vulkan::Usage::eSampledFragment, { vk::PipelineStageFlagBits2::eFragmentShader, vk::AccessFlagBits2::eShaderRead, vk::ImageLayout::eShaderReadOnlyOptimal } },
    { Vulkan::Usage::eTransferWrite, { vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferWrite, vk::ImageLayout::eTransferDstOptimal } },
    { Vulkan::Usage::eComputeWrite, { vk::PipelineStageFlagBits2::eComputeShader, vk::AccessFlagBits2::eShaderWrite, vk::ImageLayout::eGeneral } },
};
}

namespace Vulkan {
std::tuple<vk::PipelineStageFlags2, vk::AccessFlags2, vk::ImageLayout> MapUsageToVulkan(Vulkan::Usage usage) {
    if (!g_usageState.contains(usage)) {
        assert(false && "Unsupported layout transition");
        return { vk::PipelineStageFlagBits2::eAllCommands, vk::AccessFlagBits2::eMemoryRead | vk::AccessFlagBits2::eMemoryWrite, vk::ImageLayout::eUndefined };
    }

    return g_usageState.at(usage);
}

CCommandBuffer::CCommandBuffer(const vk::raii::CommandBuffer& commandBuffer) :
    m_handle(&commandBuffer)
{}

void CCommandBuffer::PipelineBarrier(std::vector<ImageBarrierInfo> imageBarriers) const {
    if (imageBarriers.size() == 0) return;

    std::vector<vk::ImageMemoryBarrier2> barriers;
    barriers.reserve(imageBarriers.size());

    for (const auto& b : imageBarriers) {
        vk::ImageMemoryBarrier2 barrier {};
        barrier.image = *b.m_image;
        barrier.subresourceRange = b.m_image.FullRange();

        std::tie(barrier.srcStageMask, barrier.srcAccessMask, barrier.oldLayout) = MapUsageToVulkan(b.m_oldUsage);
        std::tie(barrier.dstStageMask, barrier.dstAccessMask, barrier.newLayout) = MapUsageToVulkan(b.m_newUsage);

        barriers.push_back(barrier);
    }

    vk::DependencyInfo dependencyInfo {};
    dependencyInfo.imageMemoryBarrierCount = static_cast<uint32_t>(barriers.size());
    dependencyInfo.pImageMemoryBarriers = barriers.data();

    m_handle->pipelineBarrier2(dependencyInfo);
}

void CCommandBuffer::ReleaseOwnership(
    const CImage& image,
    std::uint32_t srcQueue,
    std::uint32_t dstQueue,
    Usage oldUsage,
    Usage newUsage
) const {
    vk::ImageMemoryBarrier2 barrier {};
    barrier.image = *image;
    barrier.subresourceRange = image.FullRange();
    barrier.srcQueueFamilyIndex = srcQueue;
    barrier.dstQueueFamilyIndex = dstQueue;

    std::tie(barrier.srcStageMask, barrier.srcAccessMask, barrier.oldLayout) = MapUsageToVulkan(oldUsage);
    std::tie(std::ignore, std::ignore, barrier.newLayout) = MapUsageToVulkan(newUsage);

    vk::DependencyInfo dependencyInfo {};
    dependencyInfo.imageMemoryBarrierCount = 1;
    dependencyInfo.pImageMemoryBarriers = &barrier;

    m_handle->pipelineBarrier2(dependencyInfo);
}

void CCommandBuffer::AcquireOwnership(
    CImage& image,
    std::uint32_t srcQueue,
    std::uint32_t dstQueue,
    Usage oldUsage,
    Usage newUsage
) const {
    vk::ImageMemoryBarrier2 barrier {};
    barrier.image = *image;
    barrier.subresourceRange = image.FullRange();
    barrier.srcQueueFamilyIndex = srcQueue;
    barrier.dstQueueFamilyIndex = dstQueue;

    std::tie(std::ignore, std::ignore, barrier.oldLayout) = MapUsageToVulkan(oldUsage);
    std::tie(barrier.dstStageMask, barrier.dstAccessMask, barrier.newLayout) = MapUsageToVulkan(newUsage);

    vk::DependencyInfo dependencyInfo {};
    dependencyInfo.imageMemoryBarrierCount = 1;
    dependencyInfo.pImageMemoryBarriers = &barrier;

    m_handle->pipelineBarrier2(dependencyInfo);
}

void CCommandBuffer::Copy(const CImage& dst, const CBuffer& src) const {
    vk::BufferImageCopy region;
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = dst.ArrayLevels();
    region.imageOffset = vk::Offset3D { 0, 0, 0 };
    region.imageExtent = dst.Extent();

    m_handle->copyBufferToImage(*src, *dst, vk::ImageLayout::eTransferDstOptimal, region);
}
}
