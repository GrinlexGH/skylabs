#pragma once
#include <skylabs/core/render/vulkan/render_pass.hpp>
#include <skylabs/core/render/vulkan/command_recording/command_buffer.hpp>
#include <skylabs/core/render/vulkan/pipeline/graphics_pipeline.hpp>
#include <skylabs/core/render/vulkan/submesh.hpp>

import skylabs.vulkan.resources;

namespace Vulkan {
class CPostProcessPass
{
public:
    explicit CPostProcessPass(std::nullptr_t) {}
    explicit CPostProcessPass(const CreationTools& context,
        const InFlight<CImage>& inAttachment,
        vk::Format swapchainFormat
    );
    CPostProcessPass(const CPostProcessPass&) = delete;
    CPostProcessPass(CPostProcessPass&&) noexcept = default;
    CPostProcessPass& operator=(const CPostProcessPass&) = delete;
    CPostProcessPass& operator=(CPostProcessPass&&) noexcept = default;
    ~CPostProcessPass() = default;

    void Draw(const CCommandBuffer& cmd, const CImage& swapchainImage);
    void Resize(const InFlight<CImage>& inAttachment);

private:
    const CContext* m_context = nullptr;
    const CInFlightContext* m_inFlightContext = nullptr;

    CSampler m_sampler { nullptr };

    InFlight<vk::raii::DescriptorSet> m_swapchainDescriptorSet { nullptr };
    CGraphicsPipeline m_pipelineSwapchain { nullptr };
};
}
