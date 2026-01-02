#include <skylabs/core/render/vulkan/legacy_render_pass.hpp>

namespace {
vk::ImageLayout ToLayout(const Vulkan::ImageUsage usage) {
    switch (usage) {
        case Vulkan::ImageUsage::eShaderRead: return vk::ImageLayout::eShaderReadOnlyOptimal;
        case Vulkan::ImageUsage::eSwapchainPresent: return vk::ImageLayout::ePresentSrcKHR;
    }
    std::unreachable();
}

vk::AttachmentStoreOp ToStoreOp(const Vulkan::ImageUsage usage) {
    switch (usage) {
        default: return vk::AttachmentStoreOp::eStore;
    }
}
}

namespace Vulkan {
CLegacyRenderPass::CLegacyRenderPass(const CContext& context, const CRenderPassDescription& description) {
    if (description.m_colorImages.empty()) {
        throw std::runtime_error("No render targets provided");
    }

    // Create attachment reference & description vectors
    vk::Extent2D extent;
    std::vector<vk::ImageView> views;
    std::vector<vk::AttachmentDescription> descriptions;
    std::vector<vk::AttachmentReference> colorReferences;
    for (const auto& [image, usage] : description.m_colorImages) {
        if (!image)
            continue;

        const vk::Extent2D imageExtent { image->GetExtent().width, image->GetExtent().height };
        if (extent == vk::Extent2D {}) {
            extent = imageExtent;
        } else if (extent != imageExtent) {
            Log::Debug("Image not corresponding size ({}x{}). Skipping.", extent.width, extent.height);
            continue;
        }

        vk::AttachmentDescription attachmentDescription {};
        attachmentDescription.format = image->GetFormat();
        attachmentDescription.samples = vk::SampleCountFlagBits::e1;
        attachmentDescription.loadOp = vk::AttachmentLoadOp::eClear;
        attachmentDescription.storeOp = ToStoreOp(usage);
        attachmentDescription.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
        attachmentDescription.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
        attachmentDescription.initialLayout = vk::ImageLayout::eUndefined;
        attachmentDescription.finalLayout = ToLayout(usage);

        vk::AttachmentReference attachmentReference {};
        attachmentReference.attachment = static_cast<std::uint32_t>(views.size());
        attachmentReference.layout = vk::ImageLayout::eColorAttachmentOptimal;

        views.push_back(image->GetView());
        descriptions.push_back(attachmentDescription);
        colorReferences.push_back(attachmentReference);
    }

    std::optional<vk::AttachmentReference> depthReference;
    if (description.m_depthImage) {
        vk::AttachmentDescription attachmentDescription {};
        attachmentDescription.format = description.m_depthImage->GetFormat();
        attachmentDescription.samples = vk::SampleCountFlagBits::e1;
        attachmentDescription.loadOp = vk::AttachmentLoadOp::eClear;
        attachmentDescription.storeOp = vk::AttachmentStoreOp::eDontCare;
        attachmentDescription.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
        attachmentDescription.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
        attachmentDescription.initialLayout = vk::ImageLayout::eUndefined;
        attachmentDescription.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

        vk::AttachmentReference attachmentReference {};
        attachmentReference.attachment = static_cast<uint32_t>(views.size());
        attachmentReference.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

        views.push_back(description.m_depthImage->GetView());
        descriptions.push_back(attachmentDescription);
        depthReference = attachmentReference;
    }

    vk::SubpassDescription subpassDescription {};
    subpassDescription.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
    subpassDescription.inputAttachmentCount = 0;
    subpassDescription.pInputAttachments = nullptr;
    subpassDescription.colorAttachmentCount = static_cast<std::uint32_t>(colorReferences.size());
    subpassDescription.pColorAttachments = colorReferences.data();
    subpassDescription.preserveAttachmentCount = 0;
    subpassDescription.pPreserveAttachments = nullptr;
    subpassDescription.pResolveAttachments = nullptr;
    subpassDescription.pDepthStencilAttachment = depthReference.has_value() ? std::to_address(depthReference) : nullptr;

    vk::SubpassDependency dependency {};
    dependency.srcSubpass = vk::SubpassExternal;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dependency.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
    dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;

    vk::RenderPassCreateInfo renderPassInfo {};
    renderPassInfo.attachmentCount = static_cast<std::uint32_t>(descriptions.size());
    renderPassInfo.pAttachments = descriptions.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpassDescription;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;
    m_renderPass = context.GetDevice()->createRenderPass(renderPassInfo);

    vk::FramebufferCreateInfo framebufferInfo {};
    framebufferInfo.renderPass = m_renderPass;
    framebufferInfo.attachmentCount = static_cast<std::uint32_t>(views.size());
    framebufferInfo.pAttachments = views.data();
    framebufferInfo.width = extent.width;
    framebufferInfo.height = extent.height;
    framebufferInfo.layers = 1;
    m_framebuffer = vk::raii::Framebuffer { *context.GetDevice(), framebufferInfo };
}
}
