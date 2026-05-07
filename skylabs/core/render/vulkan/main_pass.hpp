#pragma once
#include <skylabs/core/render/vulkan/renderer_context.hpp>
#include <skylabs/core/render/vulkan/resources/buffer.hpp>
#include <skylabs/core/render/vulkan/resources/sampler.hpp>
#include <skylabs/core/render/vulkan/command_buffer.hpp>
#include <skylabs/core/render/vulkan/pipeline/graphics_pipeline.hpp>
#include <skylabs/core/render/vulkan/submesh.hpp>

namespace Vulkan {
struct CMVP {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

class CMainPass
{
public:
    explicit CMainPass(std::nullptr_t) {}
    explicit CMainPass(CRendererContext& context);
    CMainPass(const CMainPass&) = delete;
    CMainPass(CMainPass&&) noexcept = default;
    CMainPass& operator=(const CMainPass&) = delete;
    CMainPass& operator=(CMainPass&&) noexcept = default;
    ~CMainPass() = default;

    void WriteDescriptors(const std::vector<CImage>& textures);
    void Draw(
        const CCommandBuffer& cmd,
        const CBuffer& vertexBuffer,
        const CBuffer& indexBuffer,
        std::span<const SubMesh> meshes
    );
    void Resize();

    InFlight<CImage>& MainAttachment() { return m_mainColor; }
    InFlight<CImage>& MainMSAAAttachment() { return m_mainColorMSAA; }
    InFlight<CImage>& DepthMSAAAttachment() { return m_mainDepthMSAA; }
    InFlight<CBuffer>& MVP() { return m_mvp; }

private:
    CRendererContext* m_rendererContext = nullptr;

    CSampler m_nearestSampler { nullptr };

    InFlight<CImage> m_mainColor { nullptr };
    InFlight<CImage> m_mainColorMSAA { nullptr };
    InFlight<CImage> m_mainDepthMSAA { nullptr };

    InFlight<CBuffer> m_mvp { nullptr };

    InFlight<vk::raii::DescriptorSet> m_mainDescriptorSet { nullptr };

    CGraphicsPipeline m_pipeline { nullptr };
};
}
