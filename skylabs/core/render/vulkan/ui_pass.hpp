#pragma once
#include <skylabs/core/render/vulkan/renderer_context.hpp>
#include <skylabs/core/render/vulkan/command_buffer.hpp>
#include <skylabs/core/render/vulkan/pipeline/graphics_pipeline.hpp>
#include <skylabs/core/render/vulkan/submesh.hpp>
#include <skylabs/core/render/vulkan/resources/sampler.hpp>

namespace Vulkan {
class CUIPass
{
public:
    explicit CUIPass(std::nullptr_t) {}
    explicit CUIPass(CRendererContext& context, const InFlight<CImage>& inAttachment);
    CUIPass(const CUIPass&) = delete;
    CUIPass(CUIPass&&) noexcept = default;
    CUIPass& operator=(const CUIPass&) = delete;
    CUIPass& operator=(CUIPass&&) noexcept = default;
    ~CUIPass() = default;

    void WriteDescriptors(const CImage& texture);
    void Draw(const CCommandBuffer& cmd, const CImage& texture);
    void Resize(const InFlight<CImage>& inAttachment);

    InFlight<CImage>& MainAttachment() { return m_uiColor; }

private:
    CRendererContext* m_rendererContext = nullptr;

    CSampler m_sampler { nullptr };

    InFlight<CImage> m_uiColor { nullptr };
    InFlight<vk::raii::DescriptorSet> m_uiDescriptorSet { nullptr };
    CGraphicsPipeline m_pipelineUI { nullptr };
};
}
