#include "render_pass.hpp"

namespace Vulkan {
CRenderPass::CRenderPass(const CRenderContext* context) : m_context(context) {
    vk::RenderPassCreateInfo createInfo {};
    createInfo.attachmentCount = 1;
    // createInfo.pAttachments = &colorAttachment;
    createInfo.subpassCount = 1;
    // createInfo.pSubpasses = &subpasses;
    createInfo.dependencyCount = 1;
    // createInfo.pDependencies = &dependencies;

    m_context->GetDevice()->GetHandle().createRenderPass(createInfo);
}
}
