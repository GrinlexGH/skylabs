#pragma once
import vulkan;

namespace Vulkan {
enum class Usage : std::uint8_t
{
    eNone,
    eColorAttachment,
    eDepthWrite,
    eDepthRead,
    eSampledFragment,
    eVertexRead,
    eComputeWrite,
    eTransferRead,
    eTransferWrite,
    ePresent
};

std::tuple<vk::PipelineStageFlags2, vk::AccessFlags2, vk::ImageLayout> MapUsageToVulkan(Usage usage);

struct ResourceSyncState
{
    Usage m_usage = Usage::eNone;
    std::uint32_t m_queue = vk::QueueFamilyIgnored;
};
}
