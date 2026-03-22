#include <skylabs/core/render/vulkan/command_buffer.hpp>

#include <frozen/map.h>

namespace {
constexpr frozen::map<vk::ImageLayout, std::tuple<vk::PipelineStageFlags2, vk::AccessFlags2>, 7> g_layoutUsage {
    { vk::ImageLayout::eUndefined, { vk::PipelineStageFlagBits2::eNone, vk::AccessFlagBits2::eNone } },
    { vk::ImageLayout::ePresentSrcKHR, { vk::PipelineStageFlagBits2::eNone, vk::AccessFlagBits2::eNone } },
    { vk::ImageLayout::eColorAttachmentOptimal, { vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::AccessFlagBits2::eColorAttachmentWrite } },
    { vk::ImageLayout::eDepthStencilAttachmentOptimal, {
        vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
        vk::AccessFlagBits2::eDepthStencilAttachmentWrite
    } },
    { vk::ImageLayout::eShaderReadOnlyOptimal, { vk::PipelineStageFlagBits2::eFragmentShader, vk::AccessFlagBits2::eShaderRead } },
    { vk::ImageLayout::eTransferDstOptimal, { vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferWrite } },
    { vk::ImageLayout::eGeneral, { vk::PipelineStageFlagBits2::eComputeShader, vk::AccessFlagBits2::eShaderWrite } },
};

vk::ImageMemoryBarrier2 GetBarrierData(
    const vk::ImageLayout oldLayout,
    const vk::ImageLayout newLayout,
    const std::uint32_t srcQueue = vk::QueueFamilyIgnored,
    const std::uint32_t dstQueue = vk::QueueFamilyIgnored,
    const Vulkan::BarrierType type = Vulkan::BarrierType::Regular
) {
    auto getStageMask = [](const vk::ImageLayout layout) -> std::tuple<vk::PipelineStageFlags2, vk::AccessFlags2> {
        if (!g_layoutUsage.contains(layout)) {
            assert(false && "Unsupported layout transition");
            return { vk::PipelineStageFlagBits2::eAllCommands, vk::AccessFlagBits2::eMemoryRead | vk::AccessFlagBits2::eMemoryWrite };
        }
        return g_layoutUsage.at(layout);
    };

    vk::ImageMemoryBarrier2 barrier {};
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = srcQueue;
    barrier.dstQueueFamilyIndex = dstQueue;

    auto [srcStage, srcAccess] = getStageMask(oldLayout);
    auto [dstStage, dstAccess] = getStageMask(newLayout);

    switch (type) {
        case Vulkan::BarrierType::Regular:
            barrier.srcStageMask = srcStage;
            barrier.srcAccessMask = srcAccess;
            barrier.dstStageMask = dstStage;
            barrier.dstAccessMask = dstAccess;
            break;

        case Vulkan::BarrierType::Release:
            barrier.srcStageMask = srcStage;
            barrier.srcAccessMask = srcAccess;
            barrier.dstStageMask = vk::PipelineStageFlagBits2::eNone;
            barrier.dstAccessMask = vk::AccessFlagBits2::eNone;
            break;

        case Vulkan::BarrierType::Acquire:
            barrier.srcStageMask = vk::PipelineStageFlagBits2::eNone;
            barrier.srcAccessMask = vk::AccessFlagBits2::eNone;
            barrier.dstStageMask = dstStage;
            barrier.dstAccessMask = dstAccess;
            break;
    }

    return barrier;
}
}

namespace Vulkan {
ImageBarrierInfo::ImageBarrierInfo(
    CImage& img,
    vk::PipelineStageFlags2 dstStage, vk::AccessFlags2 dstAccess,
    vk::ImageLayout newLayout,
    std::uint32_t srcQueue, std::uint32_t dstQueue
) {
    m_sourceCImage = &img;
    m_barrier.image = *img;
    m_barrier.srcStageMask = img.SyncState().m_stage;
    m_barrier.srcAccessMask = img.SyncState().m_access;
    m_barrier.dstStageMask = dstStage;
    m_barrier.dstAccessMask = dstAccess;
    m_barrier.oldLayout = img.SyncState().m_layout;
    m_barrier.newLayout = newLayout;
    m_barrier.srcQueueFamilyIndex = srcQueue;
    m_barrier.dstQueueFamilyIndex = dstQueue;
    m_barrier.subresourceRange = img.FullRange();
}

ImageBarrierInfo::ImageBarrierInfo(vk::ImageMemoryBarrier2 barrier) : m_barrier(barrier) {}

CCommandBuffer::CCommandBuffer(const vk::raii::CommandBuffer& commandBuffer) :
    m_handle(&commandBuffer)
{}

void CCommandBuffer::PipelineBarrier(std::vector<ImageBarrierInfo> imageBarriers) const {
    if (imageBarriers.size() == 0) return;

    std::vector<vk::ImageMemoryBarrier2> barriers;
    barriers.reserve(imageBarriers.size());

    for (const auto& t : imageBarriers) {
        barriers.push_back(t.m_barrier);

        if (t.m_sourceCImage) {
            t.m_sourceCImage->m_syncState.m_stage = t.m_barrier.dstStageMask;
            t.m_sourceCImage->m_syncState.m_access = t.m_barrier.dstAccessMask;
            t.m_sourceCImage->m_syncState.m_layout = t.m_barrier.newLayout;
        }
    }

    vk::DependencyInfo dependencyInfo {};
    dependencyInfo.imageMemoryBarrierCount = static_cast<uint32_t>(barriers.size());
    dependencyInfo.pImageMemoryBarriers = barriers.data();

    m_handle->pipelineBarrier2(dependencyInfo);
}

vk::ImageMemoryBarrier2 CCommandBuffer::CreateImageBarrier(
    vk::Image image,
    vk::ImageLayout oldLayout,
    vk::ImageLayout newLayout,
    vk::ImageAspectFlags aspectMask,
    uint32_t mipLevels,
    uint32_t arrayLayers
) const {
    auto barrier = GetBarrierData(oldLayout, newLayout);

    barrier.image = image;
    barrier.subresourceRange.aspectMask = aspectMask;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = mipLevels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = arrayLayers;

    return barrier;
}

void CCommandBuffer::ReleaseOwnership(
    const CImage& image,
    uint32_t srcQueue,
    uint32_t dstQueue,
    vk::ImageLayout newLayout
) const {
    auto barrier = GetBarrierData(image.m_syncState.m_layout, newLayout, srcQueue, dstQueue, BarrierType::Release);

    barrier.image = *image;
    barrier.subresourceRange = image.FullRange();

    vk::DependencyInfo dependencyInfo {};
    dependencyInfo.imageMemoryBarrierCount = 1;
    dependencyInfo.pImageMemoryBarriers = &barrier;

    m_handle->pipelineBarrier2(dependencyInfo);
}

void CCommandBuffer::AcquireOwnership(
    CImage& image,
    uint32_t srcQueue,
    uint32_t dstQueue,
    vk::ImageLayout newLayout
) const {
    auto barrier = GetBarrierData(image.m_syncState.m_layout, newLayout, srcQueue, dstQueue, BarrierType::Acquire);

    barrier.image = *image;
    barrier.subresourceRange = image.FullRange();

    vk::DependencyInfo dependencyInfo {};
    dependencyInfo.imageMemoryBarrierCount = 1;
    dependencyInfo.pImageMemoryBarriers = &barrier;

    m_handle->pipelineBarrier2(dependencyInfo);

    image.m_syncState.m_layout = newLayout;
}
}
