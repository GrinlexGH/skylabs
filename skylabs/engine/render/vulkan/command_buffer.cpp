#include "skylabs/engine/render/vulkan/command_buffer.hpp"

namespace {
struct UsageState {
    vk::PipelineStageFlags2 stage;
    vk::AccessFlags2 access;
    vk::ImageLayout layout;

    void ApplyAsSrc(vk::ImageMemoryBarrier2& barrier) const noexcept {
        barrier.srcStageMask = stage;
        barrier.srcAccessMask = access;
        barrier.oldLayout = layout;
    }

    void ApplyAsSrc(vk::BufferMemoryBarrier2& barrier) const noexcept {
        barrier.srcStageMask = stage;
        barrier.srcAccessMask = access;
    }

    void ApplyAsDst(vk::ImageMemoryBarrier2& barrier) const noexcept {
        barrier.dstStageMask = stage;
        barrier.dstAccessMask = access;
        barrier.newLayout = layout;
    }

    void ApplyAsDst(vk::BufferMemoryBarrier2& barrier) const noexcept {
        barrier.dstStageMask = stage;
        barrier.dstAccessMask = access;
    }
};

UsageState GetUsageState(const sk::render::vulkan::Usage usage) {
    using namespace sk::render::vulkan;

    switch (usage) {
        case Usage::eNone:
            return { .stage = vk::PipelineStageFlagBits2::eNone,
                     .access = vk::AccessFlagBits2::eNone,
                     .layout = vk::ImageLayout::eUndefined };
        case Usage::eSwapchainAcquire:
            return { .stage = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                     .access = vk::AccessFlagBits2::eNone,
                     .layout = vk::ImageLayout::eUndefined };
        case Usage::ePresent:
            return { .stage = vk::PipelineStageFlagBits2::eNone,
                     .access = vk::AccessFlagBits2::eNone,
                     .layout = vk::ImageLayout::ePresentSrcKHR };
        case Usage::eColorAttachment:
            return { .stage = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                     .access = vk::AccessFlagBits2::eColorAttachmentWrite,
                     .layout = vk::ImageLayout::eColorAttachmentOptimal };
        case Usage::eDepthWrite:
            return { .stage = vk::PipelineStageFlagBits2::eEarlyFragmentTests |
                              vk::PipelineStageFlagBits2::eLateFragmentTests,
                     .access = vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
                     .layout = vk::ImageLayout::eDepthStencilAttachmentOptimal };
        case Usage::eDepthRead:
            return { .stage = vk::PipelineStageFlagBits2::eEarlyFragmentTests |
                              vk::PipelineStageFlagBits2::eLateFragmentTests,
                     .access = vk::AccessFlagBits2::eDepthStencilAttachmentRead,
                     .layout = vk::ImageLayout::eDepthStencilReadOnlyOptimal };
        case Usage::eSampledFragment:
            return { .stage = vk::PipelineStageFlagBits2::eFragmentShader,
                     .access = vk::AccessFlagBits2::eShaderRead,
                     .layout = vk::ImageLayout::eShaderReadOnlyOptimal };
        case Usage::eTransferWrite:
            return { .stage = vk::PipelineStageFlagBits2::eTransfer,
                     .access = vk::AccessFlagBits2::eTransferWrite,
                     .layout = vk::ImageLayout::eTransferDstOptimal };
        case Usage::eTransferRead:
            return { .stage = vk::PipelineStageFlagBits2::eTransfer,
                     .access = vk::AccessFlagBits2::eTransferRead,
                     .layout = vk::ImageLayout::eTransferSrcOptimal };
        case Usage::eComputeWrite:
            return { .stage = vk::PipelineStageFlagBits2::eComputeShader,
                     .access = vk::AccessFlagBits2::eShaderWrite,
                     .layout = vk::ImageLayout::eGeneral };
        case Usage::eVertexRead:
            return { .stage = vk::PipelineStageFlagBits2::eVertexShader,
                     .access = vk::AccessFlagBits2::eShaderRead,
                     .layout = vk::ImageLayout::eShaderReadOnlyOptimal };
        default:
            return { };
    }
}
}

namespace sk::render::vulkan {
CommandBuffer::CommandBuffer(const vk::raii::Device& device, vk::raii::CommandBuffer&& commandBuffer)
    : m_device(&device), m_handle(std::move(commandBuffer)) { }

void CommandBuffer::PipelineBarrier(
    const std::vector<std::variant<ImageBarrier, BufferBarrier>>& barriers) const {
    if (barriers.empty()) return;

    std::vector<vk::BufferMemoryBarrier2> bufBarriers;
    std::vector<vk::ImageMemoryBarrier2> imgBarriers;
    bufBarriers.reserve(barriers.size());
    imgBarriers.reserve(barriers.size());

    for (const auto& barrier : barriers) {
        if (std::holds_alternative<ImageBarrier>(barrier)) {
            const auto& [image, range, oldUsage, newUsage, type, srcQueue, dstQueue] =
                std::get<ImageBarrier>(barrier);

            vk::ImageMemoryBarrier2 imageBarrier { };
            imageBarrier.image = *image;
            imageBarrier.subresourceRange = range;
            imageBarrier.srcQueueFamilyIndex = srcQueue;
            imageBarrier.dstQueueFamilyIndex = dstQueue;

            GetUsageState(oldUsage).ApplyAsSrc(imageBarrier);
            GetUsageState(newUsage).ApplyAsDst(imageBarrier);

            if (type == BarrierType::eRegular) {
                imageBarrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
                imageBarrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;
            } else if (type == BarrierType::eRelease) {
                imageBarrier.dstStageMask = vk::PipelineStageFlagBits2::eNone;
                imageBarrier.dstAccessMask = vk::AccessFlagBits2::eNone;
            } else if (type == BarrierType::eAcquire) {
                imageBarrier.srcStageMask = vk::PipelineStageFlagBits2::eNone;
                imageBarrier.srcAccessMask = vk::AccessFlagBits2::eNone;
            }

            imgBarriers.push_back(imageBarrier);
        } else if (std::holds_alternative<BufferBarrier>(barrier)) {
            const auto& [buffer, oldUsage, newUsage, type, srcQueue, dstQueue] =
                std::get<BufferBarrier>(barrier);

            vk::BufferMemoryBarrier2 bufferBarrier { };
            bufferBarrier.buffer = *buffer;
            bufferBarrier.size = buffer.Size();
            bufferBarrier.offset = 0;
            bufferBarrier.srcQueueFamilyIndex = srcQueue;
            bufferBarrier.dstQueueFamilyIndex = dstQueue;

            GetUsageState(oldUsage).ApplyAsSrc(bufferBarrier);
            GetUsageState(newUsage).ApplyAsDst(bufferBarrier);

            if (type == BarrierType::eRegular) {
                bufferBarrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
                bufferBarrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;
            } else if (type == BarrierType::eRelease) {
                bufferBarrier.dstStageMask = vk::PipelineStageFlagBits2::eNone;
                bufferBarrier.dstAccessMask = vk::AccessFlagBits2::eNone;
            } else if (type == BarrierType::eAcquire) {
                bufferBarrier.srcStageMask = vk::PipelineStageFlagBits2::eNone;
                bufferBarrier.srcAccessMask = vk::AccessFlagBits2::eNone;
            }

            bufBarriers.push_back(bufferBarrier);
        }
    }

    vk::DependencyInfo dependencyInfo { };
    dependencyInfo.imageMemoryBarrierCount = static_cast<std::uint32_t>(imgBarriers.size());
    dependencyInfo.pImageMemoryBarriers = imgBarriers.data();
    dependencyInfo.bufferMemoryBarrierCount = static_cast<std::uint32_t>(bufBarriers.size());
    dependencyInfo.pBufferMemoryBarriers = bufBarriers.data();

    m_handle.pipelineBarrier2(dependencyInfo);
}

void CommandBuffer::Copy(const Buffer& src, const Image& dst) const {
    vk::BufferImageCopy region;
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = dst.AspectFlags();
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = dst.ArrayLevels();
    region.imageOffset = vk::Offset3D { 0, 0, 0 };
    region.imageExtent = dst.Extent();

    m_handle.copyBufferToImage(*src, *dst, vk::ImageLayout::eTransferDstOptimal, region);
}

void CommandBuffer::Copy(const Buffer& src, const Buffer& dst, const vk::DeviceSize size,
                         const BufferCopyOffsets& offsets) const {
    vk::BufferCopy copyRegion;
    copyRegion.srcOffset = offsets.srcOffset;
    copyRegion.dstOffset = offsets.dstOffset;
    copyRegion.size = size;

    m_handle.copyBuffer(*src, *dst, copyRegion);
}

void CommandBuffer::GenerateMipmaps(const Image& image, const Usage srcUsage,
                                    const Usage dstUsage) const {
    std::int32_t mipWidth = static_cast<std::int32_t>(image.Extent().width);
    std::int32_t mipHeight = static_cast<std::int32_t>(image.Extent().height);

    for (std::uint32_t i = 1; i < image.MipLevels(); i++) {
        PipelineBarrier({ ImageBarrier {
            .image = image,
            .range = vk::ImageSubresourceRange { image.AspectFlags(), i - 1, 1, 0, image.ArrayLevels() },
            .oldUsage = (i == 1) ? srcUsage : Usage::eTransferWrite,
            .newUsage = Usage::eTransferRead,
        } });

        vk::ImageBlit blit { };
        blit.srcSubresource = { image.AspectFlags(), i - 1, 0, image.ArrayLevels() };
        blit.srcOffsets[1] = vk::Offset3D { mipWidth, mipHeight, 1 };

        blit.dstSubresource = { image.AspectFlags(), i, 0, image.ArrayLevels() };
        blit.dstOffsets[1] =
            vk::Offset3D { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };

        m_handle.blitImage(*image, vk::ImageLayout::eTransferSrcOptimal, *image,
                           vk::ImageLayout::eTransferDstOptimal, { blit }, vk::Filter::eLinear);

        PipelineBarrier({ ImageBarrier {
            .image = image,
            .range = vk::ImageSubresourceRange { image.AspectFlags(), i - 1, 1, 0, image.ArrayLevels() },
            .oldUsage = Usage::eTransferRead,
            .newUsage = dstUsage,
        } });

        if (mipWidth > 1) mipWidth /= 2;
        if (mipHeight > 1) mipHeight /= 2;
    }

    PipelineBarrier({ ImageBarrier {
        .image = image,
        .range = vk::ImageSubresourceRange { image.AspectFlags(), image.MipLevels() - 1, 1, 0,
                                             image.ArrayLevels() },
        .oldUsage = Usage::eTransferWrite,
        .newUsage = dstUsage,
    } });
}
}
