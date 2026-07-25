#include <skylabs/core/render/vulkan/command_recording/command_buffer.hpp>

namespace Vulkan {
CCommandBuffer::CCommandBuffer(const vk::raii::Device& device, vk::raii::CommandBuffer&& commandBuffer) :
    m_device(&device), m_handle(std::move(commandBuffer))
{}

void CCommandBuffer::PipelineBarrier(const std::vector<std::variant<ImageBarrier, BufferBarrier>>& barriers) const {
    if (barriers.empty()) return;

    std::vector<vk::BufferMemoryBarrier2> bufBarriers;
    std::vector<vk::ImageMemoryBarrier2> imgBarriers;
    bufBarriers.reserve(barriers.size());
    imgBarriers.reserve(barriers.size());

    for (const auto& barrier : barriers) {
        if (std::holds_alternative<ImageBarrier>(barrier)) {
            const auto& [
                image,
                range,
                oldUsage,
                newUsage,
                type,
                srcQueue,
                dstQueue
            ] = std::get<ImageBarrier>(barrier);

            vk::ImageMemoryBarrier2 imageBarrier {};
            imageBarrier.image = *image;
            imageBarrier.subresourceRange = range;
            imageBarrier.srcQueueFamilyIndex = srcQueue;
            imageBarrier.dstQueueFamilyIndex = dstQueue;

            std::tie(
                imageBarrier.srcStageMask,
                imageBarrier.srcAccessMask,
                imageBarrier.oldLayout
            ) = MapUsageToVulkan(oldUsage);

            std::tie(
                imageBarrier.dstStageMask,
                imageBarrier.dstAccessMask,
                imageBarrier.newLayout
            ) = MapUsageToVulkan(newUsage);

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
            const auto& [
                buffer,
                oldUsage,
                newUsage,
                type,
                srcQueue,
                dstQueue
            ] = std::get<BufferBarrier>(barrier);

            vk::BufferMemoryBarrier2 bufferBarrier {};
            bufferBarrier.buffer = *buffer;
            bufferBarrier.size = buffer.Size();
            bufferBarrier.offset = 0;
            bufferBarrier.srcQueueFamilyIndex = srcQueue;
            bufferBarrier.dstQueueFamilyIndex = dstQueue;

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

            std::tie(bufferBarrier.srcStageMask, bufferBarrier.srcAccessMask, std::ignore) = MapUsageToVulkan(oldUsage);
            std::tie(bufferBarrier.dstStageMask, bufferBarrier.dstAccessMask, std::ignore) = MapUsageToVulkan(newUsage);
            bufBarriers.push_back(bufferBarrier);
        }
    }

    vk::DependencyInfo dependencyInfo {};
    dependencyInfo.imageMemoryBarrierCount = static_cast<std::uint32_t>(imgBarriers.size());
    dependencyInfo.pImageMemoryBarriers = imgBarriers.data();
    dependencyInfo.bufferMemoryBarrierCount = static_cast<std::uint32_t>(bufBarriers.size());
    dependencyInfo.pBufferMemoryBarriers = bufBarriers.data();

    m_handle.pipelineBarrier2(dependencyInfo);
}

void CCommandBuffer::GenerateMipmaps(const CImage& image, const Usage srcUsage, const Usage dstUsage) const {
    std::int32_t mipWidth = static_cast<std::int32_t>(image.Extent().width);
    std::int32_t mipHeight = static_cast<std::int32_t>(image.Extent().height);

    for (std::uint32_t i = 1; i < image.MipLevels(); i++) {
        PipelineBarrier({ ImageBarrier {
            .m_image = image,
            .m_range = vk::ImageSubresourceRange { image.AspectFlags(), i - 1, 1, 0, image.ArrayLevels() },
            .m_oldUsage = (i == 1) ? srcUsage : Usage::eTransferWrite,
            .m_newUsage = Usage::eTransferRead,
        }});

        vk::ImageBlit blit {};
        blit.srcSubresource = { image.AspectFlags(), i - 1, 0, image.ArrayLevels() };
        blit.srcOffsets[1] = vk::Offset3D { mipWidth, mipHeight, 1 };

        blit.dstSubresource = { image.AspectFlags(), i, 0, image.ArrayLevels() };
        blit.dstOffsets[1] = vk::Offset3D {
            mipWidth > 1 ? mipWidth / 2 : 1,
            mipHeight > 1 ? mipHeight / 2 : 1,
            1
        };

        m_handle.blitImage(
            *image, vk::ImageLayout::eTransferSrcOptimal,
            *image, vk::ImageLayout::eTransferDstOptimal,
            { blit },
            vk::Filter::eLinear
        );

        PipelineBarrier({ ImageBarrier {
            .m_image = image,
            .m_range = vk::ImageSubresourceRange { image.AspectFlags(), i - 1, 1, 0, image.ArrayLevels() },
            .m_oldUsage = Usage::eTransferRead,
            .m_newUsage = dstUsage,
        }});

        if (mipWidth > 1) mipWidth /= 2;
        if (mipHeight > 1) mipHeight /= 2;
    }

    PipelineBarrier({ ImageBarrier {
        .m_image = image,
        .m_range = vk::ImageSubresourceRange { image.AspectFlags(), image.MipLevels() - 1, 1, 0, image.ArrayLevels() },
        .m_oldUsage = Usage::eTransferWrite,
        .m_newUsage = dstUsage,
    }});
}

void CCommandBuffer::Copy(const CBuffer& src, const CImage& dst) const {
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

void CCommandBuffer::Copy(const CBuffer& src, const CBuffer& dst, const vk::DeviceSize size, const BufferCopyOffsets offsets) const {
    vk::BufferCopy copyRegion {};
    copyRegion.srcOffset = offsets.m_srcOffset;
    copyRegion.dstOffset = offsets.m_dstOffset;
    copyRegion.size = size;

    m_handle.copyBuffer(*src, *dst, copyRegion);
}
}
