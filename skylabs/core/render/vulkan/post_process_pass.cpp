#include "skylabs/core/render/vulkan/post_process_pass.hpp"
#include "skylabs/base/utils.hpp"
#include "skylabs/core/render/vulkan/pipeline/descriptor_writer.hpp"

namespace sk::render::vulkan {
PostProcessPass::PostProcessPass(const Device& device, const InFlightContext& inFlightContext,
                                 PipelineLayoutCache& pipelineLayoutCache,
                                 DescriptorLayoutCache& descriptorLayoutCache,
                                 DescriptorAllocator& descriptorAllocator,
                                 const filesystem::Filesystem& filesystem,
                                 const InFlight<Image>& inAttachment, const vk::Format swapchainFormat)
    : m_device(&*device), m_inFlightContext(&inFlightContext) {
    m_sampler = Sampler { device, { .filtering = vk::Filter::eNearest } };

    // Descriptor sets
    const vk::raii::DescriptorSetLayout& swapchainSetLayout = descriptorLayoutCache.GetLayout(
        { { 0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment } });

    m_swapchainDescriptorSet =
        InFlight { inFlightContext, descriptorAllocator.Allocate(std::vector(
                                        inFlightContext.FrameCount(), *swapchainSetLayout)) };

    // Write descriptors
    DescriptorWriter descriptorWriter { *device };
    for (const auto i : utils::Range(inFlightContext.FrameCount())) {
        descriptorWriter.Clear();
        descriptorWriter
            .WriteImage(0, inAttachment[i].View(), *m_sampler, vk::ImageLayout::eShaderReadOnlyOptimal,
                        vk::DescriptorType::eCombinedImageSampler)
            .UpdateSet(*m_swapchainDescriptorSet[i]);
    }

    // Shaders
    const Shader vertexShaderSwapchain(
        *device, vk::ShaderStageFlagBits::eVertex,
        filesystem.LoadAsVector32("res://shaders/shaderSwapchain.vert.spv"));
    const Shader fragmentShaderSwapchain(
        *device, vk::ShaderStageFlagBits::eFragment,
        filesystem.LoadAsVector32("res://shaders/shaderSwapchain.frag.spv"));

    const vk::raii::PipelineLayout& swapchainPipelineLayout =
        pipelineLayoutCache.GetLayout({ { *swapchainSetLayout }, { } });

    // Pipeline
    std::array swapchainColorFormats { swapchainFormat };
    m_pipelineSwapchain =
        GraphicsPipeline { *device,
                           { .layout = swapchainPipelineLayout,
                             .shaders = { &vertexShaderSwapchain, &fragmentShaderSwapchain },
                             .vertexBindings = { },
                             .renderingInfo = { { }, swapchainColorFormats } } };
}

void PostProcessPass::Draw(const CommandBuffer& cmd, const Image& swapchainImage) {
    vk::RenderingAttachmentInfo swapchainAttachInfo { };
    swapchainAttachInfo.imageView = *swapchainImage.View();
    swapchainAttachInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
    swapchainAttachInfo.loadOp = vk::AttachmentLoadOp::eClear;
    swapchainAttachInfo.storeOp = vk::AttachmentStoreOp::eStore;
    swapchainAttachInfo.clearValue.color = std::array { 0.005f, 0.005f, 0.005f, 1.0f };

    vk::RenderingInfo swapchainRenderInfo { };
    swapchainRenderInfo.renderArea = vk::Rect2D { { 0, 0 }, swapchainImage.Extent2D() };
    swapchainRenderInfo.layerCount = 1;
    swapchainRenderInfo.colorAttachmentCount = 1;
    swapchainRenderInfo.pColorAttachments = &swapchainAttachInfo;

    cmd->beginRendering(swapchainRenderInfo);
    cmd->bindPipeline(vk::PipelineBindPoint::eGraphics, *m_pipelineSwapchain);

    cmd->setViewport(0, { { 0.0f, 0.0f, static_cast<float>(swapchainImage.Extent().width),
                            static_cast<float>(swapchainImage.Extent().height), 0.0f, 1.0f } });
    cmd->setScissor(0, { { { 0, 0 }, swapchainImage.Extent2D() } });

    cmd->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_pipelineSwapchain.Layout(), 0,
                            *m_swapchainDescriptorSet.Get(), { });
    cmd->draw(3, 1, 0, 0);
    cmd->endRendering();
}

void PostProcessPass::Resize(const InFlight<Image>& inAttachment) {
    for (const auto i : utils::Range(m_inFlightContext->FrameCount())) {
        DescriptorWriter descriptorWriter { *m_device };
        descriptorWriter
            .WriteImage(0, inAttachment[i].View(), *m_sampler, vk::ImageLayout::eShaderReadOnlyOptimal,
                        vk::DescriptorType::eCombinedImageSampler)
            .UpdateSet(*m_swapchainDescriptorSet[i]);
    }
}
}
