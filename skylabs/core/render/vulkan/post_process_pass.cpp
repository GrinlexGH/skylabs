#include <skylabs/core/render/vulkan/post_process_pass.hpp>
#include <skylabs/core/render/vulkan/pipeline/descriptor_writer.hpp>

namespace Vulkan {
CPostProcessPass::CPostProcessPass(
    CRendererContext& context,
    const InFlight<CImage>& inAttachment
) : m_rendererContext(&context)
{
    auto& inFlightContext = context.InFlightContext();
    auto& deviceContext = context.DeviceContext();
    auto& descriptorLayoutCache = context.DescriptorLayoutCache();
    auto& descriptorAllocator = context.DescriptorAllocator();
    auto& pipelineLayoutCache = context.PipelineLayoutCache();
    auto& swapchain = context.Swapchain();
    auto [width, height] = swapchain.Extent();

    m_sampler = CSampler { deviceContext, { .m_filtering = vk::Filter::eNearest } };

    // Descriptor sets
    const vk::raii::DescriptorSetLayout& swapchainSetLayout = descriptorLayoutCache.GetLayout({
        { 0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment }
    });

    m_swapchainDescriptorSet = InFlight{ inFlightContext,
        descriptorAllocator.Allocate(std::vector(inFlightContext.FrameCount(), *swapchainSetLayout))
    };

    // Write descriptors
    CDescriptorWriter descriptorWriter { deviceContext };
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
        deviceContext, vk::ShaderStageFlagBits::eVertex, "res://shaders/shaderSwapchain.vert.spv"
    );
    const CShader fragmentShaderSwapchain(
        deviceContext, vk::ShaderStageFlagBits::eFragment, "res://shaders/shaderSwapchain.frag.spv"
    );

    const vk::raii::PipelineLayout& swapchainPipelineLayout = pipelineLayoutCache.GetLayout({
        { *swapchainSetLayout }
    });

    // Pipeline
    std::array swapchainColorFormats { swapchain.SurfaceFormat().format };
    m_pipelineSwapchain = CGraphicsPipeline { deviceContext, {
        .m_layout = swapchainPipelineLayout,
        .m_shaders = { &vertexShaderSwapchain, &fragmentShaderSwapchain },
        .m_renderingInfo = { {}, swapchainColorFormats }
    }};
}

void CPostProcessPass::Draw(const CCommandBuffer& cmd, const std::uint32_t imageIndex) {
    auto& swapchain = m_rendererContext->Swapchain();

    vk::RenderingAttachmentInfo swapchainAttachInfo {};
    swapchainAttachInfo.imageView = *swapchain.Images()[imageIndex].View();
    swapchainAttachInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
    swapchainAttachInfo.loadOp = vk::AttachmentLoadOp::eClear;
    swapchainAttachInfo.storeOp = vk::AttachmentStoreOp::eStore;
    swapchainAttachInfo.clearValue.color = std::array { 0.0f, 0.0f, 0.0f, 1.0f };

    vk::RenderingInfo swapchainRenderInfo {};
    swapchainRenderInfo.renderArea = vk::Rect2D { { 0, 0 }, swapchain.Extent() };
    swapchainRenderInfo.layerCount = 1;
    swapchainRenderInfo.colorAttachmentCount = 1;
    swapchainRenderInfo.pColorAttachments = &swapchainAttachInfo;

    cmd->beginRendering(swapchainRenderInfo);
    cmd->bindPipeline(vk::PipelineBindPoint::eGraphics, *m_pipelineSwapchain);

    cmd->setViewport(0, {
        {
            0.0f, 0.0f,
            static_cast<float>(swapchain.Extent().width), static_cast<float>(swapchain.Extent().height),
            0.0f, 1.0f
        }
    });
    cmd->setScissor(0, { { { 0, 0 }, swapchain.Extent() } });

    cmd->bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics, m_pipelineSwapchain.Layout(), 0,
        *m_swapchainDescriptorSet.Get(),{}
    );
    cmd->draw(3, 1, 0, 0);
    cmd->endRendering();
}

void CPostProcessPass::Resize(const InFlight<CImage>& inAttachment) {
    for (const auto i : Utils::Range(m_rendererContext->InFlightContext().FrameCount())) {
        CDescriptorWriter descriptorWriter { m_rendererContext->DeviceContext() };
        descriptorWriter
            .WriteImage(0, inAttachment[i].View(), *m_sampler, vk::ImageLayout::eShaderReadOnlyOptimal, vk::DescriptorType::eCombinedImageSampler)
            .UpdateSet(*m_swapchainDescriptorSet[i]);
    }
}
}
