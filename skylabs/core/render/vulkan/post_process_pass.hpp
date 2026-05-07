#pragma once
#include <skylabs/core/render/vulkan/renderer_context.hpp>
#include <skylabs/core/render/vulkan/command_buffer.hpp>
#include <skylabs/core/render/vulkan/pipeline/graphics_pipeline.hpp>
#include <skylabs/core/render/vulkan/submesh.hpp>
#include <skylabs/core/render/vulkan/resources/sampler.hpp>

namespace Vulkan {
class CPostProcessPass
{
public:
    explicit CPostProcessPass(std::nullptr_t) {}
    explicit CPostProcessPass(CRendererContext& context, const InFlight<CImage>& inAttachment);
    CPostProcessPass(const CPostProcessPass&) = delete;
    CPostProcessPass(CPostProcessPass&&) noexcept = default;
    CPostProcessPass& operator=(const CPostProcessPass&) = delete;
    CPostProcessPass& operator=(CPostProcessPass&&) noexcept = default;
    ~CPostProcessPass() = default;

    void Draw(const CCommandBuffer& cmd, std::uint32_t imageIndex);
    void Resize(const InFlight<CImage>& inAttachment);

private:
    CRendererContext* m_rendererContext = nullptr;

    CSampler m_sampler { nullptr };

    InFlight<vk::raii::DescriptorSet> m_swapchainDescriptorSet { nullptr };
    CGraphicsPipeline m_pipelineSwapchain { nullptr };
};
}
