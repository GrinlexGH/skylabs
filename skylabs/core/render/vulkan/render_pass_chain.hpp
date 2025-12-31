#pragma once
#include <skylabs/core/render/vulkan/render_pass.hpp>

namespace Vulkan {
class CRenderPassChain
{
public:
    explicit CRenderPassChain(std::nullptr_t) {}
    explicit CRenderPassChain(const CContext& context) : m_context(&context) {}

    auto AddPass(const CRenderPass& pass) -> void;

    [[nodiscard]] auto GetRenderPass() const -> const vk::raii::RenderPass& { return m_renderPass; }
    [[nodiscard]] auto GetFrameBuffer() const -> const vk::raii::Framebuffer& { return m_frameBuffer; }

private:
    const CContext* m_context = nullptr;

    vk::raii::RenderPass m_renderPass { nullptr };
    vk::raii::Framebuffer m_frameBuffer { nullptr };
};
}
