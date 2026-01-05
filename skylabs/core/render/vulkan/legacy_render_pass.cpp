#include <skylabs/core/render/vulkan/legacy_render_pass.hpp>

namespace {
vk::ImageLayout ToLayout(const Vulkan::ImageUsage usage) {
    switch (usage) {
        case Vulkan::ImageUsage::eMSAAWrite: return vk::ImageLayout::eColorAttachmentOptimal;
        case Vulkan::ImageUsage::eShaderRead: return vk::ImageLayout::eShaderReadOnlyOptimal;
        case Vulkan::ImageUsage::eSwapchainPresent: return vk::ImageLayout::ePresentSrcKHR;
    }
    std::unreachable();
}

vk::AttachmentStoreOp ToStoreOp(const Vulkan::ImageUsage usage) {
    switch (usage) {
        case Vulkan::ImageUsage::eMSAAWrite: return vk::AttachmentStoreOp::eDontCare;
        default: return vk::AttachmentStoreOp::eStore;
    }
}
}

namespace Vulkan {
CLegacyRenderPass::CLegacyRenderPass(const CContext& context, const CRenderPassDescription& description) {
    assert(
       description.m_resolveImages.size() == description.m_colorImages.size() ||
       description.m_resolveImages.empty()
    );

    // Create attachment reference & description vectors
    vk::Extent2D extent;
    std::vector<vk::ImageView> views;
    std::vector<vk::AttachmentDescription> descriptions;
    std::vector<vk::AttachmentReference> colorReferences;

    auto addAttachment = [&](
        const CImage* image,
        const vk::SampleCountFlagBits sampleCount,
        const vk::AttachmentLoadOp loadOp,
        const vk::AttachmentStoreOp storeOp,
        const vk::ImageLayout finalLayout,
        const vk::ImageLayout referenceLayout
    ) -> vk::AttachmentReference {
        vk::AttachmentDescription attachmentDescription {};
        attachmentDescription.format = image->Format();
        attachmentDescription.samples = sampleCount;
        attachmentDescription.loadOp = loadOp;
        attachmentDescription.storeOp = storeOp;
        attachmentDescription.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
        attachmentDescription.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
        attachmentDescription.initialLayout = vk::ImageLayout::eUndefined;
        attachmentDescription.finalLayout = finalLayout;

        vk::AttachmentReference attachmentReference {};
        attachmentReference.attachment = static_cast<std::uint32_t>(views.size());
        attachmentReference.layout = referenceLayout;

        views.push_back(image->View());
        descriptions.push_back(attachmentDescription);
        return attachmentReference;
    };

    auto validateImage = [&](const CImage* image) -> bool {
        if (!image)
            return false;

        // Validate extent
        const vk::Extent2D imageExtent { image->Extent().width, image->Extent().height };
        if (extent == vk::Extent2D {}) {
            extent = imageExtent;
        } else if (extent != imageExtent) {
            Log::Debug("Image not corresponding size ({}x{}). Skipping.", extent.width, extent.height);
            return false;
        }

        return true;
    };

    // Create descriptions for color images
    for (const auto& [image, usage] : description.m_colorImages) {
        if (!validateImage(image))
            continue;

        colorReferences.push_back(addAttachment(
            image, image->SampleCount(),
            vk::AttachmentLoadOp::eClear, ToStoreOp(usage),
            ToLayout(usage), vk::ImageLayout::eColorAttachmentOptimal
        ));
    }

    // MSAA-after images
    std::vector<vk::AttachmentReference> resolveReferences;
    for (const auto& [image, usage] : description.m_resolveImages) {
        if (!validateImage(image))
            continue;

        resolveReferences.push_back(addAttachment(
            image, vk::SampleCountFlagBits::e1,
            vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eStore,
            ToLayout(usage), vk::ImageLayout::eColorAttachmentOptimal
        ));
    }

    // Depth
    std::optional<vk::AttachmentReference> depthReference;
    if (validateImage(description.m_depthImage)) {
        depthReference = addAttachment(
            description.m_depthImage, description.m_depthImage->SampleCount(),
            vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eDontCare,
            vk::ImageLayout::eDepthStencilAttachmentOptimal, vk::ImageLayout::eDepthStencilAttachmentOptimal
        );
    }

    vk::SubpassDescription subpassDescription {};
    subpassDescription.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
    subpassDescription.inputAttachmentCount = 0;
    subpassDescription.pInputAttachments = nullptr;
    subpassDescription.colorAttachmentCount = static_cast<std::uint32_t>(colorReferences.size());
    subpassDescription.pColorAttachments = !colorReferences.empty() ? colorReferences.data() : nullptr;
    subpassDescription.preserveAttachmentCount = 0;
    subpassDescription.pPreserveAttachments = nullptr;
    subpassDescription.pResolveAttachments = !resolveReferences.empty() ? resolveReferences.data() : nullptr;
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
    m_renderPass = context.Device()->createRenderPass(renderPassInfo);

    vk::FramebufferCreateInfo framebufferInfo {};
    framebufferInfo.renderPass = m_renderPass;
    framebufferInfo.attachmentCount = static_cast<std::uint32_t>(views.size());
    framebufferInfo.pAttachments = views.data();
    framebufferInfo.width = extent.width;
    framebufferInfo.height = extent.height;
    framebufferInfo.layers = 1;
    m_framebuffer = vk::raii::Framebuffer { *context.Device(), framebufferInfo };
}
}
