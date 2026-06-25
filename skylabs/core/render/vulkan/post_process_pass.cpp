#include <skylabs/core/render/vulkan/post_process_pass.hpp>
#include <skylabs/core/render/vulkan/pipeline/descriptor_writer.hpp>
#include <skylabs/public/utils.hpp>

namespace Vulkan {
CPostProcessPass::CPostProcessPass(
    const CreationTools& creationTools,
    const InFlight<CImage>& inAttachment,
    vk::Format swapchainFormat
) : m_context(&creationTools.m_context), m_inFlightContext(&creationTools.m_inFlightContext)
{
    auto& inFlightContext = creationTools.m_inFlightContext;
    auto& context = creationTools.m_context;
    auto& descriptorLayoutCache = creationTools.m_descriptorLayoutCache;
    auto& descriptorAllocator = creationTools.m_descriptorAllocator;
    auto& pipelineLayoutCache = creationTools.m_pipelineLayoutCache;

    m_sampler = CSampler { context, { .m_filtering = vk::Filter::eNearest } };

    // Descriptor sets
    const vk::raii::DescriptorSetLayout& swapchainSetLayout = descriptorLayoutCache.GetLayout({
        { 0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment }
    });

    m_swapchainDescriptorSet = InFlight { inFlightContext,
        descriptorAllocator.Allocate(std::vector(inFlightContext.FrameCount(), *swapchainSetLayout))
    };

    // Write descriptors
    CDescriptorWriter descriptorWriter { context };
    for (const auto i : Utils::Range(inFlightContext.FrameCount())) {
        descriptorWriter.Clear();
        descriptorWriter
            .WriteImage(
                0, inAttachment[i].View(), *m_sampler,
                vk::ImageLayout::eShaderReadOnlyOptimal, vk::DescriptorType::eCombinedImageSampler
            ).UpdateSet(*m_swapchainDescriptorSet[i]);
    }

    // Shaders
    const CShader vertexShaderSwapchain(
        context, vk::ShaderStageFlagBits::eVertex, "res://shaders/shaderSwapchain.vert.spv"
    );
    const CShader fragmentShaderSwapchain(
        context, vk::ShaderStageFlagBits::eFragment, "res://shaders/shaderSwapchain.frag.spv"
    );

    const vk::raii::PipelineLayout& swapchainPipelineLayout = pipelineLayoutCache.GetLayout({
        { *swapchainSetLayout }
    });

    // Pipeline
    std::array swapchainColorFormats { swapchainFormat };
    m_pipelineSwapchain = CGraphicsPipeline { context, {
        .m_layout = swapchainPipelineLayout,
        .m_shaders = { &vertexShaderSwapchain, &fragmentShaderSwapchain },
        .m_renderingInfo = { {}, swapchainColorFormats }
    }};
}

void CPostProcessPass::Draw(const CCommandBuffer& cmd, const CImage& swapchainImage) {
    vk::RenderingAttachmentInfo swapchainAttachInfo {};
    swapchainAttachInfo.imageView = *swapchainImage.View();
    swapchainAttachInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
    swapchainAttachInfo.loadOp = vk::AttachmentLoadOp::eClear;
    swapchainAttachInfo.storeOp = vk::AttachmentStoreOp::eStore;
    swapchainAttachInfo.clearValue.color = std::array { 0.0f, 0.0f, 0.0f, 1.0f };

    vk::RenderingInfo swapchainRenderInfo {};
    swapchainRenderInfo.renderArea = vk::Rect2D { { 0, 0 }, swapchainImage.Extent2D() };
    swapchainRenderInfo.layerCount = 1;
    swapchainRenderInfo.colorAttachmentCount = 1;
    swapchainRenderInfo.pColorAttachments = &swapchainAttachInfo;

    cmd->beginRendering(swapchainRenderInfo);
    cmd->bindPipeline(vk::PipelineBindPoint::eGraphics, *m_pipelineSwapchain);

    cmd->setViewport(0, {
        {
            0.0f, 0.0f,
            static_cast<float>(swapchainImage.Extent().width), static_cast<float>(swapchainImage.Extent().height),
            0.0f, 1.0f
        }
    });
    cmd->setScissor(0, { { { 0, 0 }, swapchainImage.Extent2D() } });

    cmd->bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics, m_pipelineSwapchain.Layout(), 0,
        *m_swapchainDescriptorSet.Get(),{}
    );
    cmd->draw(3, 1, 0, 0);
    cmd->endRendering();
}

void CPostProcessPass::Resize(const InFlight<CImage>& inAttachment) {
    for (const auto i : Utils::Range(m_inFlightContext->FrameCount())) {
        CDescriptorWriter descriptorWriter { *m_context };
        descriptorWriter
            .WriteImage(0, inAttachment[i].View(), *m_sampler, vk::ImageLayout::eShaderReadOnlyOptimal, vk::DescriptorType::eCombinedImageSampler)
            .UpdateSet(*m_swapchainDescriptorSet[i]);
    }
}
}
