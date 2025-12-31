#include <skylabs/core/render/vulkan/render_pass_chain.hpp>

namespace Vulkan {
void CRenderPassChain::AddPass(const CRenderPass& pass) {
    const auto [
        extent,
        views,
        descriptions,
        colorReferences,
        depthReference
    ] = pass.FinalizeAttachments();

    vk::SubpassDescription subpass {};
    subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
    subpass.inputAttachmentCount = 0;
    subpass.pInputAttachments = nullptr;
    subpass.colorAttachmentCount = static_cast<std::uint32_t>(colorReferences.size());
    subpass.pColorAttachments = colorReferences.data();
    subpass.preserveAttachmentCount = 0;
    subpass.pPreserveAttachments = nullptr;
    subpass.pResolveAttachments = nullptr;
    subpass.pDepthStencilAttachment = depthReference.has_value() ? &*depthReference : nullptr;

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
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;
    m_renderPass = m_context->GetDevice().GetHandle().createRenderPass(renderPassInfo);

    vk::FramebufferCreateInfo framebufferInfo {};
    framebufferInfo.renderPass = m_renderPass;
    framebufferInfo.attachmentCount = static_cast<std::uint32_t>(views.size());
    framebufferInfo.pAttachments = views.data();
    framebufferInfo.width = extent.width;
    framebufferInfo.height = extent.height;
    framebufferInfo.layers = 1;
    m_frameBuffer = vk::raii::Framebuffer { *m_context->GetDevice(), framebufferInfo };
}
}
