#include <skylabs/core/render/vulkan/render_graph/graph.hpp>

namespace Vulkan::RG {
void CRenderGraph::AddPass(Pass pass) {
    m_passes.push_back(std::move(pass));
}

void CRenderGraph::Execute() {
    int graphicsI = 0;
    int computeI = 0;
    for (auto& p : m_passes) {
        auto cmd = p.m_type == PassType::eGraphics ? m_graphicsQueues.PrimaryBuffers()[m_inFlightIndex * 3 + graphicsI++] : m_computeQueues.PrimaryBuffers()[m_inFlightIndex + computeI++];
        std::vector<ImageBarrierInfo> imagePrepareBarriers;
        std::vector<ImageBarrierInfo> imageAfterBarriers;

        for (auto& req : p.m_requirements) {
            if (req.m_image.SyncState().m_usage != req.m_usage) {
                imagePrepareBarriers.emplace_back(req.m_image, req.m_image.SyncState().m_usage, req.m_usage);

                auto ss = req.m_image.SyncState();
                ss.m_usage = req.m_usage;
                req.m_image.SetSyncState(ss);
            }

            if (req.m_usageAfter != Usage::eNone && (req.m_image.SyncState().m_usage != req.m_usageAfter)) {
                imageAfterBarriers.emplace_back(req.m_image, req.m_image.SyncState().m_usage, req.m_usageAfter);

                auto ss = req.m_image.SyncState();
                ss.m_usage = req.m_usageAfter;
                req.m_image.SetSyncState(ss);
            }
        }

        cmd->reset();
        cmd->begin({});

        if (!imagePrepareBarriers.empty()) {
            cmd.PipelineBarrier(imagePrepareBarriers);
        }

        p.m_execute(cmd);

        if (!imageAfterBarriers.empty()) {
            cmd.PipelineBarrier(imageAfterBarriers);
        }

        cmd->end();

        std::vector<vk::Semaphore> waitSems;
        std::vector<vk::PipelineStageFlags> waitStages;
        waitSems.reserve(p.m_sync.m_waitSemaphores.size());
        waitStages.reserve(p.m_sync.m_waitSemaphores.size());
        for (auto& waitSem : p.m_sync.m_waitSemaphores) {
            waitSems.push_back(waitSem.m_semaphore);
            waitStages.push_back(waitSem.m_stage);
        }

        vk::SubmitInfo finalSubmit {};
        finalSubmit.waitSemaphoreCount = static_cast<std::uint32_t>(p.m_sync.m_waitSemaphores.size());
        finalSubmit.pWaitSemaphores = waitSems.data();
        finalSubmit.pWaitDstStageMask = waitStages.data();
        finalSubmit.commandBufferCount = 1;
        finalSubmit.pCommandBuffers = &**cmd;
        finalSubmit.signalSemaphoreCount = p.m_sync.m_signalSemaphores.size();
        finalSubmit.pSignalSemaphores = p.m_sync.m_signalSemaphores.data();
        if (p.m_type == PassType::eGraphics) {
            m_context->Device().GraphicsQueue()->submit(finalSubmit, p.m_sync.m_fence);
        } else {
            m_context->Device().ComputeQueue()->submit(finalSubmit, p.m_sync.m_fence);
        }
    }

    m_inFlightIndex = (m_inFlightIndex + 1) % m_inFlightCount;
}
}
