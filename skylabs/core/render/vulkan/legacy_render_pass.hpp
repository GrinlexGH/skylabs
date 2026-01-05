#pragma once
#include <skylabs/core/render/vulkan/context/context.hpp>
#include <skylabs/core/render/vulkan/render_target.hpp>

namespace Vulkan {
class CLegacyRenderPass
{
public:
    explicit CLegacyRenderPass(std::nullptr_t) {}
    explicit CLegacyRenderPass(const CContext& context, const CRenderPassDescription& description);
    CLegacyRenderPass(const CLegacyRenderPass&) = delete;
    CLegacyRenderPass(CLegacyRenderPass&&) noexcept = default;
    CLegacyRenderPass& operator=(const CLegacyRenderPass&) = delete;
    CLegacyRenderPass& operator=(CLegacyRenderPass&&) noexcept = default;
    ~CLegacyRenderPass() = default;

    [[nodiscard]] auto RenderPass() const noexcept -> vk::RenderPass { return *m_renderPass; }
    [[nodiscard]] auto Framebuffer() const noexcept -> vk::Framebuffer { return *m_framebuffer; }

private:
    vk::raii::RenderPass m_renderPass { nullptr };
    vk::raii::Framebuffer m_framebuffer { nullptr };
};
}
