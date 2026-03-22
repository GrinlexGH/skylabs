#include <skylabs/core/render/vulkan/render_graph/graph.hpp>

#include <frozen/map.h>

namespace {
constexpr frozen::map<Vulkan::Usage, std::tuple<
    vk::PipelineStageFlags2,
    vk::AccessFlags2,
    vk::ImageLayout>, 5> g_usageUsage {
    { Vulkan::Usage::eNone, { vk::PipelineStageFlagBits2::eNone, vk::AccessFlagBits2::eNone, vk::ImageLayout::eUndefined } },
    { Vulkan::Usage::ePresent, { vk::PipelineStageFlagBits2::eNone, vk::AccessFlagBits2::eNone, vk::ImageLayout::ePresentSrcKHR } },
    { Vulkan::Usage::eColorAttachment, { vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::AccessFlagBits2::eColorAttachmentWrite, vk::ImageLayout::eColorAttachmentOptimal } },
    { Vulkan::Usage::eDepthWrite, {
        vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
        vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
        vk::ImageLayout::eDepthStencilAttachmentOptimal
    } },
    { Vulkan::Usage::eSampledFragment, { vk::PipelineStageFlagBits2::eFragmentShader, vk::AccessFlagBits2::eShaderRead, vk::ImageLayout::eShaderReadOnlyOptimal } },
};

std::tuple<
    vk::PipelineStageFlags2,
    vk::AccessFlags2,
    vk::ImageLayout>
MapUsageToVulkan(Vulkan::Usage usage) {
    if (!g_usageUsage.contains(usage)) {
        assert(false && "Unsupported layout transition");
        return { vk::PipelineStageFlagBits2::eAllCommands, vk::AccessFlagBits2::eMemoryRead | vk::AccessFlagBits2::eMemoryWrite, vk::ImageLayout::eUndefined };
    }
    return g_usageUsage.at(usage);
}
}

namespace Vulkan::RG {
void CRenderGraph::AddPass(Pass pass) {
    m_passes.push_back(std::move(pass));
}

void CRenderGraph::Execute() {
    for (auto& p : m_passes) {
        std::vector<Vulkan::ImageBarrierInfo> syncBarriers;

        for (auto& req : p.m_requirements) {
            if (req.m_image->SyncState().m_usage != req.m_usage) {
                
                auto [stage, access, layout] = MapUsageToVulkan(req.m_usage);

                syncBarriers.push_back({ *req.m_image, stage, access, layout });

                auto ss = req.m_image->SyncState();
                ss.m_usage = req.m_usage;
                req.m_image->SetSyncState(ss);
            }
        }

        p.m_cmd->reset();
        p.m_cmd->begin({});

        if (!syncBarriers.empty()) {
            p.m_cmd.PipelineBarrier(syncBarriers);
        }

        p.m_execute();
    }
}
}