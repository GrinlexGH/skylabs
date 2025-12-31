#include <skylabs/core/render/vulkan/render_pass.hpp>

namespace {
vk::ImageLayout ToVkLayout(const Vulkan::ImageUsage usage) {
    switch (usage) {
        case Vulkan::ImageUsage::eShaderRead: return vk::ImageLayout::eShaderReadOnlyOptimal;
        case Vulkan::ImageUsage::eDepth: return vk::ImageLayout::eDepthStencilAttachmentOptimal;
        case Vulkan::ImageUsage::ePresent: return vk::ImageLayout::ePresentSrcKHR;
    }
    std::unreachable();
}
}

namespace Vulkan {
void CRenderPass::AddOutImage(const CImage& image, const ImageUsage usage) {
    if (m_extent == vk::Extent2D {}) {
        m_extent = { { image.GetExtent().width, image.GetExtent().height } };
    } else if (m_extent != vk::Extent2D { image.GetExtent().width, image.GetExtent().height } ) {
        Log::Debug("Image not corresponding size ({}x{}). Skipping.", m_extent.width, m_extent.height);
        return;
    }

    m_alreadyFinalized = false;

    vk::AttachmentDescription attachmentDescription {};
    attachmentDescription.format = image.GetFormat();
    attachmentDescription.samples = vk::SampleCountFlagBits::e1;
    attachmentDescription.loadOp = vk::AttachmentLoadOp::eClear;
    attachmentDescription.storeOp = usage == ImageUsage::eDepth ? vk::AttachmentStoreOp::eDontCare : vk::AttachmentStoreOp::eStore;
    attachmentDescription.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    attachmentDescription.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    attachmentDescription.initialLayout = vk::ImageLayout::eUndefined;
    attachmentDescription.finalLayout = ToVkLayout(usage);

    vk::AttachmentReference attachmentReference {};
    attachmentReference.attachment = static_cast<std::uint32_t>(m_colorAttachmentViews.size());
    attachmentReference.layout = usage == ImageUsage::eDepth ? vk::ImageLayout::eDepthStencilAttachmentOptimal : vk::ImageLayout::eColorAttachmentOptimal;

    if (usage == ImageUsage::eDepth) {
        m_depthAttachmentDescription = attachmentDescription;
        m_depthAttachmentReference = attachmentReference;
        m_depthAttachmentView = image.GetView();
    } else {
        m_colorAttachmentDescriptions.push_back(attachmentDescription);
        m_colorAttachmentReferences.push_back(attachmentReference);
        m_colorAttachmentViews.push_back(image.GetView());
    }
}

CRenderPass::CFinalizedAttachments CRenderPass::FinalizeAttachments() const {
    static CFinalizedAttachments finalized;

    if (m_alreadyFinalized == true) {
        return finalized;
    }

    finalized.m_extent = m_extent;
    finalized.m_views.clear();
    finalized.m_descriptions.clear();
    finalized.m_colorReferences.clear();
    finalized.m_depthReference.reset();

    finalized.m_views.reserve(m_colorAttachmentViews.size() + 1);
    finalized.m_views = m_colorAttachmentViews;
    finalized.m_descriptions.reserve(m_colorAttachmentDescriptions.size() + 1);
    finalized.m_descriptions = m_colorAttachmentDescriptions;
    finalized.m_colorReferences = m_colorAttachmentReferences;

    if (m_depthAttachmentDescription.has_value()) {
        finalized.m_depthReference = m_depthAttachmentReference;
        finalized.m_depthReference->attachment = static_cast<std::uint32_t>(finalized.m_views.size());
        finalized.m_descriptions.push_back(*m_depthAttachmentDescription);
        finalized.m_views.push_back(*m_depthAttachmentView);
    }

    m_alreadyFinalized = true;
    return finalized;
}
}
