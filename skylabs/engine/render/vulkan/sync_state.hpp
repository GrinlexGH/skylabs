#pragma once
#include <vulkan/vulkan.hpp>

namespace sk::render::vulkan {
enum class Usage : std::uint8_t {
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

struct ResourceSyncState {
    Usage usage = Usage::eNone;
    std::uint32_t queue = vk::QueueFamilyIgnored;
};
}
