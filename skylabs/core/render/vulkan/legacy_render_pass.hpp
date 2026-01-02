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

    [[nodiscard]] auto GetRenderPass() const noexcept -> vk::RenderPass { return *m_renderPass; }
    [[nodiscard]] auto GetFramebuffer() const noexcept -> vk::Framebuffer { return *m_framebuffer; }

private:
    vk::Extent2D m_extent;

    std::vector<vk::ImageView> m_views;
    std::vector<vk::AttachmentDescription> m_descriptions;
    std::vector<vk::AttachmentReference> m_colorReferences;
    std::optional<vk::AttachmentReference> m_depthReference;

    vk::raii::RenderPass m_renderPass { nullptr };
    vk::raii::Framebuffer m_framebuffer { nullptr };
};
}
