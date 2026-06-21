#include <skylabs/core/render/vulkan/command_recording/sync_state.hpp>

namespace {
constexpr frozen::map<Vulkan::Usage, std::tuple<vk::PipelineStageFlags2, vk::AccessFlags2, vk::ImageLayout>, 9> g_usageState {
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
    { Vulkan::Usage::eTransferRead, { vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferRead, vk::ImageLayout::eTransferSrcOptimal } },
    { Vulkan::Usage::eComputeWrite, { vk::PipelineStageFlagBits2::eComputeShader, vk::AccessFlagBits2::eShaderWrite, vk::ImageLayout::eGeneral } },
    { Vulkan::Usage::eVertexRead, { vk::PipelineStageFlagBits2::eVertexShader, vk::AccessFlagBits2::eShaderRead, vk::ImageLayout::eShaderReadOnlyOptimal } },
};
}

namespace Vulkan {
std::tuple<vk::PipelineStageFlags2, vk::AccessFlags2, vk::ImageLayout> MapUsageToVulkan(Usage usage) {
    if (!g_usageState.contains(usage)) {
        assert(false && "Unsupported layout transition");
        return { vk::PipelineStageFlagBits2::eAllCommands, vk::AccessFlagBits2::eMemoryRead | vk::AccessFlagBits2::eMemoryWrite, vk::ImageLayout::eUndefined };
    }

    return g_usageState.at(usage);
}
}
