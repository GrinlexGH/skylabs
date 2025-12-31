#pragma once
#include <skylabs/core/render/vulkan/memory/image.hpp>

namespace Vulkan {
enum class ImageUsage : std::uint8_t
{
    eShaderRead,
    eDepth,
    ePresent,
};

class CRenderPass
{
public:
    struct CFinalizedAttachments {
        vk::Extent2D m_extent;
        std::vector<vk::ImageView> m_views;
        std::vector<vk::AttachmentDescription> m_descriptions;
        std::vector<vk::AttachmentReference> m_colorReferences;
        std::optional<vk::AttachmentReference> m_depthReference;
    };

    auto AddOutImage(const CImage& image, ImageUsage usage) -> void;
    [[nodiscard]] auto FinalizeAttachments() const -> CFinalizedAttachments;

private:
    vk::Extent2D m_extent;

    std::vector<vk::AttachmentDescription> m_colorAttachmentDescriptions;
    std::vector<vk::AttachmentReference> m_colorAttachmentReferences;
    std::vector<vk::ImageView> m_colorAttachmentViews;

    std::optional<vk::AttachmentDescription> m_depthAttachmentDescription;
    std::optional<vk::AttachmentReference> m_depthAttachmentReference;
    std::optional<vk::ImageView> m_depthAttachmentView;

    mutable bool m_alreadyFinalized = false;
};
}
