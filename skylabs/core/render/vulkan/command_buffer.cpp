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
CCommandBuffer::CCommandBuffer(const vk::raii::CommandBuffer& commandBuffer) :
    m_handle(&commandBuffer)
{}

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

void CCommandBuffer::TransitionLayout(
    vk::Image image,
    vk::ImageLayout oldLayout,
    vk::ImageLayout newLayout,
    vk::ImageAspectFlags aspectMask,
    std::uint32_t mipLevels,
    std::uint32_t arrayLevels
) const {
    auto barrier = CreateImageBarrier(image, oldLayout, newLayout, aspectMask, mipLevels, arrayLevels);

    vk::DependencyInfo dependencyInfo {};
    dependencyInfo.imageMemoryBarrierCount = 1;
    dependencyInfo.pImageMemoryBarriers = &barrier;

    m_handle->pipelineBarrier2(dependencyInfo);
}

void CCommandBuffer::TransitionLayout(CImage& image, vk::ImageLayout newLayout) const {
    TransitionLayout(
        *image.m_handle,
        image.m_layout,
        newLayout,
        image.m_aspectFlags,
        image.m_mipLevels,
        image.m_arrayLevels
    );

    image.m_layout = newLayout;
}

void CCommandBuffer::TransitionLayout(std::initializer_list<ImageBarrierInfo> transitions) const {
    if (transitions.size() == 0) return;

    std::vector<vk::ImageMemoryBarrier2> barriers;
    barriers.reserve(transitions.size());

    for (const auto& t : transitions) {
        barriers.push_back(CreateImageBarrier(
            t.m_image,
            t.m_oldLayout,
            t.m_newLayout,
            t.m_aspectMask,
            t.m_mipLevels,
            t.m_arrayLevels
        ));

        if (t.m_sourceCImage) {
            t.m_sourceCImage->m_layout = t.m_newLayout;
        }
    }

    vk::DependencyInfo dependencyInfo {};
    dependencyInfo.imageMemoryBarrierCount = static_cast<uint32_t>(barriers.size());
    dependencyInfo.pImageMemoryBarriers = barriers.data();

    m_handle->pipelineBarrier2(dependencyInfo);
}

void CCommandBuffer::ReleaseOwnership(
    const CImage& image,
    uint32_t srcQueue,
    uint32_t dstQueue,
    vk::ImageLayout newLayout
) const {
    auto barrier = GetBarrierData(image.Layout(), newLayout, srcQueue, dstQueue, BarrierType::Release);

    barrier.image = *image;
    barrier.subresourceRange.aspectMask = image.AspectFlags();
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = image.MipLevels();
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = image.ArrayLevels();

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
    auto barrier = GetBarrierData(image.Layout(), newLayout, srcQueue, dstQueue, BarrierType::Acquire);

    barrier.image = *image;
    barrier.subresourceRange.aspectMask = image.AspectFlags();
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = image.MipLevels();
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = image.ArrayLevels();

    vk::DependencyInfo dependencyInfo {};
    dependencyInfo.imageMemoryBarrierCount = 1;
    dependencyInfo.pImageMemoryBarriers = &barrier;

    m_handle->pipelineBarrier2(dependencyInfo);

    image.m_layout = newLayout;
}

void CCommandBuffer::ReleaseOwnership(std::initializer_list<ImageBarrierInfo> releases) const {
    if (releases.size() == 0) return;

    std::vector<vk::ImageMemoryBarrier2> barriers;
    barriers.reserve(releases.size());

    for (auto& t : releases) {
        if (t.m_sourceCImage) {
            auto barrier = GetBarrierData(
                t.m_oldLayout,
                t.m_newLayout,
                t.m_srcQueue,
                t.m_dstQueue,
                BarrierType::Release
            );

            barrier.image = t.m_image;
            barrier.subresourceRange.aspectMask = t.m_aspectMask;
            barrier.subresourceRange.baseMipLevel = 0;
            barrier.subresourceRange.levelCount = t.m_arrayLevels;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = t.m_arrayLevels;

            barriers.push_back(barrier);

            t.m_sourceCImage->m_layout = t.m_newLayout;
        } else {
            vk::ImageMemoryBarrier2 barrier;
            barrier.image = t.m_image;
            barrier.srcQueueFamilyIndex = t.m_srcQueue;
            barrier.dstQueueFamilyIndex = t.m_dstQueue;
            barrier.subresourceRange.aspectMask = t.m_aspectMask;
            barrier.subresourceRange.baseMipLevel = 0;
            barrier.subresourceRange.levelCount = t.m_arrayLevels;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = t.m_arrayLevels;

            barriers.push_back(barrier);
        }
    }

    vk::DependencyInfo dependencyInfo {};
    dependencyInfo.imageMemoryBarrierCount = static_cast<uint32_t>(barriers.size());
    dependencyInfo.pImageMemoryBarriers = barriers.data();

    m_handle->pipelineBarrier2(dependencyInfo);
}

void CCommandBuffer::AcquireOwnership(std::initializer_list<ImageBarrierInfo> transfers) const {
    if (transfers.size() == 0) return;

    std::vector<vk::ImageMemoryBarrier2> barriers;
    barriers.reserve(transfers.size());

    for (auto& t : transfers) {
        if (t.m_sourceCImage) {
            auto barrier = GetBarrierData(
                t.m_oldLayout,
                t.m_newLayout,
                t.m_srcQueue,
                t.m_dstQueue,
                BarrierType::Acquire
            );

            barrier.image = t.m_image;
            barrier.subresourceRange.aspectMask = t.m_aspectMask;
            barrier.subresourceRange.baseMipLevel = 0;
            barrier.subresourceRange.levelCount = t.m_arrayLevels;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = t.m_arrayLevels;

            barriers.push_back(barrier);

            t.m_sourceCImage->m_layout = t.m_newLayout;
        } else {
            vk::ImageMemoryBarrier2 barrier;
            barrier.image = t.m_image;
            barrier.srcQueueFamilyIndex = t.m_srcQueue;
            barrier.dstQueueFamilyIndex = t.m_dstQueue;
            barrier.subresourceRange.aspectMask = t.m_aspectMask;
            barrier.subresourceRange.baseMipLevel = 0;
            barrier.subresourceRange.levelCount = t.m_arrayLevels;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = t.m_arrayLevels;

            barriers.push_back(barrier);
        }
    }

    vk::DependencyInfo dependencyInfo {};
    dependencyInfo.imageMemoryBarrierCount = static_cast<uint32_t>(barriers.size());
    dependencyInfo.pImageMemoryBarriers = barriers.data();

    m_handle->pipelineBarrier2(dependencyInfo);
}
}
