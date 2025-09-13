#include "image.hpp"

#include <map>

namespace {
auto FindMemoryType(
    const vk::raii::PhysicalDevice& physicalDevice,
    const std::uint32_t typeFilter,
    vk::MemoryPropertyFlags properties
) -> std::uint32_t {
    const static vk::PhysicalDeviceMemoryProperties memProperties = physicalDevice.getMemoryProperties();

    for (std::uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("Failed to find suitable memory type!");
}

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
CImage::CImage(std::nullptr_t) {}

CImage::CImage(
    const CContext* context,
    vk::Extent3D extent,
    vk::Format format,
    vk::ImageTiling tiling,
    vk::ImageUsageFlags usage,
    vk::ImageAspectFlags imageAspectFlags,
    vk::MemoryPropertyFlags memoryProperties
) : m_context(context) {
    const vk::raii::Device& deviceHandle = context->GetDevice().GetHandle();

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

    m_handle = deviceHandle.createImage(imageInfo);

    const vk::MemoryRequirements memRequirements = m_handle.getMemoryRequirements();

    vk::MemoryAllocateInfo allocInfo {};
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(
        context->GetPhysicalDevice()->GetHandle(),
        memRequirements.memoryTypeBits,
        memoryProperties
    );

    m_memory = deviceHandle.allocateMemory(allocInfo);
    m_handle.bindMemory(m_memory, 0);

    vk::ImageViewCreateInfo imageViewInfo {};
    imageViewInfo.image = m_handle;
    imageViewInfo.viewType = vk::ImageViewType::e2D;
    imageViewInfo.format = format;
    imageViewInfo.subresourceRange.aspectMask = imageAspectFlags;
    imageViewInfo.subresourceRange.baseMipLevel = 0;
    imageViewInfo.subresourceRange.levelCount = 1;
    imageViewInfo.subresourceRange.baseArrayLayer = 0;
    imageViewInfo.subresourceRange.layerCount = 1;

    m_view = deviceHandle.createImageView(imageViewInfo);
}

auto CImage::TransitionLayout(const vk::raii::CommandBuffer& commandBuffer, vk::ImageLayout newLayout) -> void {
    auto key = std::make_pair(m_layout, newLayout);
    auto it = GetTransitionRules().find(key);
    if (it == GetTransitionRules().end()) {
        throw std::invalid_argument("Unsupported layout transition!");
    }

    const auto& rule = it->second;

    vk::ImageMemoryBarrier2KHR barrier {};
    barrier.srcStageMask = rule.m_srcStageMask;
    barrier.srcAccessMask = rule.m_srcAccessMask;
    barrier.dstStageMask = rule.m_dstStageMask;
    barrier.dstAccessMask = rule.m_dstAccessMask;
    barrier.oldLayout = m_layout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
    barrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;
    barrier.image = m_handle;
    barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    vk::DependencyInfoKHR dependencyInfo{};
    dependencyInfo.imageMemoryBarrierCount = 1;
    dependencyInfo.pImageMemoryBarriers = &barrier;

    commandBuffer.pipelineBarrier2(dependencyInfo);

    m_layout = newLayout;
}
}
