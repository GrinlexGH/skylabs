#include <skylabs/core/render/vulkan/command_recording/command_buffer.hpp>

namespace Vulkan {
CCommandBuffer::CCommandBuffer(const CDeviceContext& context, vk::raii::CommandBuffer&& commandBuffer) :
    m_device(&*context.Device()), m_handle(std::move(commandBuffer))
{}

void CCommandBuffer::PipelineBarrier(const std::vector<std::variant<ImageBarrier, BufferBarrier>>& barriers) const {
    if (barriers.size() == 0) return;

    std::vector<vk::BufferMemoryBarrier2> bufBarriers;
    std::vector<vk::ImageMemoryBarrier2> imgBarriers;
    bufBarriers.reserve(barriers.size());
    imgBarriers.reserve(barriers.size());

    for (const auto& b : barriers) {
        if (std::holds_alternative<ImageBarrier>(b)) {
            auto& imgb = std::get<ImageBarrier>(b);

            vk::ImageMemoryBarrier2 imgBarrier {};
            imgBarrier.image = *imgb.m_image;
            imgBarrier.subresourceRange = imgb.m_range;
            imgBarrier.srcQueueFamilyIndex = imgb.srcQueue;
            imgBarrier.dstQueueFamilyIndex = imgb.dstQueue;

            std::tie(imgBarrier.srcStageMask, imgBarrier.srcAccessMask, imgBarrier.oldLayout) = MapUsageToVulkan(imgb.m_oldUsage);
            std::tie(imgBarrier.dstStageMask, imgBarrier.dstAccessMask, imgBarrier.newLayout) = MapUsageToVulkan(imgb.m_newUsage);

            if (imgb.m_type == BarrierType::eRegular) {
                imgBarrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
                imgBarrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;
            } else if (imgb.m_type == BarrierType::eRelease) {
                imgBarrier.dstStageMask = vk::PipelineStageFlagBits2::eNone;
                imgBarrier.dstAccessMask = vk::AccessFlagBits2::eNone;
            } else if (imgb.m_type == BarrierType::eAcquire) {
                imgBarrier.srcStageMask = vk::PipelineStageFlagBits2::eNone;
                imgBarrier.srcAccessMask = vk::AccessFlagBits2::eNone;
            }

            imgBarriers.push_back(imgBarrier);
        } else if (std::holds_alternative<BufferBarrier>(b)) {
            vk::BufferMemoryBarrier2 bufBarrier {};
            auto& bufb = std::get<BufferBarrier>(b);
            bufBarrier.buffer = *bufb.m_buffer;
            bufBarrier.size = bufb.m_buffer.Size();
            bufBarrier.offset = 0;
            bufBarrier.srcQueueFamilyIndex = bufb.srcQueue;
            bufBarrier.dstQueueFamilyIndex = bufb.dstQueue;

            if (bufb.m_type == BarrierType::eRelease) {
                bufBarrier.dstStageMask = vk::PipelineStageFlagBits2::eNone;
                bufBarrier.dstAccessMask = vk::AccessFlagBits2::eNone;
            } else if (bufb.m_type == BarrierType::eAcquire) {
                bufBarrier.srcStageMask = vk::PipelineStageFlagBits2::eNone;
                bufBarrier.srcAccessMask = vk::AccessFlagBits2::eNone;
            }

            std::tie(bufBarrier.srcStageMask, bufBarrier.srcAccessMask, std::ignore) = MapUsageToVulkan(bufb.m_oldUsage);
            std::tie(bufBarrier.dstStageMask, bufBarrier.dstAccessMask, std::ignore) = MapUsageToVulkan(bufb.m_newUsage);
            bufBarriers.push_back(bufBarrier);
        }
    }

    vk::DependencyInfo dependencyInfo {};
    dependencyInfo.imageMemoryBarrierCount = static_cast<std::uint32_t>(imgBarriers.size());
    dependencyInfo.pImageMemoryBarriers = imgBarriers.data();
    dependencyInfo.bufferMemoryBarrierCount = static_cast<std::uint32_t>(bufBarriers.size());
    dependencyInfo.pBufferMemoryBarriers = bufBarriers.data();

    m_handle.pipelineBarrier2(dependencyInfo);
}

void CCommandBuffer::GenerateMipmaps(const CImage& image, Usage srcUsage, Usage dstUsage) const {
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

void CCommandBuffer::Copy(const CImage& dst, const CBuffer& src) const {
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

void CCommandBuffer::Copy(const CBuffer& dst, const CBuffer& src, const vk::DeviceSize size, const BufferCopyOffsets offsets) const {
    vk::BufferCopy copyRegion {};
    copyRegion.srcOffset = offsets.m_srcOffset;
    copyRegion.dstOffset = offsets.m_dstOffset;
    copyRegion.size = size;

    m_handle.copyBuffer(*src, *dst, copyRegion);
}
}
