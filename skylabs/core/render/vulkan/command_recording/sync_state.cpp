#include <frozen/map.h>

#include "skylabs/core/render/vulkan/command_recording/sync_state.hpp"

namespace {
constexpr frozen::map<sk::render::vulkan::Usage,
                      std::tuple<vk::PipelineStageFlags2, vk::AccessFlags2, vk::ImageLayout>, 9>
    kUsageState {
        { sk::render::vulkan::Usage::eNone,
          { vk::PipelineStageFlagBits2::eNone, vk::AccessFlagBits2::eNone,
            vk::ImageLayout::eUndefined } },
        { sk::render::vulkan::Usage::ePresent,
          { vk::PipelineStageFlagBits2::eNone, vk::AccessFlagBits2::eNone,
            vk::ImageLayout::ePresentSrcKHR } },
        { sk::render::vulkan::Usage::eColorAttachment,
          { vk::PipelineStageFlagBits2::eColorAttachmentOutput,
            vk::AccessFlagBits2::eColorAttachmentWrite, vk::ImageLayout::eColorAttachmentOptimal } },
        { sk::render::vulkan::Usage::eDepthWrite,
          { vk::PipelineStageFlagBits2::eEarlyFragmentTests |
                vk::PipelineStageFlagBits2::eLateFragmentTests,
            vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
            vk::ImageLayout::eDepthStencilAttachmentOptimal } },
        { sk::render::vulkan::Usage::eSampledFragment,
          { vk::PipelineStageFlagBits2::eFragmentShader, vk::AccessFlagBits2::eShaderRead,
            vk::ImageLayout::eShaderReadOnlyOptimal } },
        { sk::render::vulkan::Usage::eTransferWrite,
          { vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferWrite,
            vk::ImageLayout::eTransferDstOptimal } },
        { sk::render::vulkan::Usage::eTransferRead,
          { vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferRead,
            vk::ImageLayout::eTransferSrcOptimal } },
        { sk::render::vulkan::Usage::eComputeWrite,
          { vk::PipelineStageFlagBits2::eComputeShader, vk::AccessFlagBits2::eShaderWrite,
            vk::ImageLayout::eGeneral } },
        { sk::render::vulkan::Usage::eVertexRead,
          { vk::PipelineStageFlagBits2::eVertexShader, vk::AccessFlagBits2::eShaderRead,
            vk::ImageLayout::eShaderReadOnlyOptimal } },
    };
}

namespace sk::render::vulkan {
std::tuple<vk::PipelineStageFlags2, vk::AccessFlags2, vk::ImageLayout> MapUsageToVulkan(
    const Usage usage) {
    if (!kUsageState.contains(usage)) {
        assert(false && "Unsupported layout transition");
        return { vk::PipelineStageFlagBits2::eAllCommands,
                 vk::AccessFlagBits2::eMemoryRead | vk::AccessFlagBits2::eMemoryWrite,
                 vk::ImageLayout::eUndefined };
    }

    return kUsageState.at(usage);
}
}
