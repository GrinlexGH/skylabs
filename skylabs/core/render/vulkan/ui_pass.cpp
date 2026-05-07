#include <skylabs/core/render/vulkan/ui_pass.hpp>
#include <skylabs/core/render/vulkan/pipeline/descriptor_writer.hpp>

struct UIConstants {
    glm::vec2 screenRes;
    glm::vec2 textRes;
};

namespace Vulkan {
CUIPass::CUIPass(CRendererContext& context, const InFlight<CImage>& inAttachment) : m_rendererContext(&context) {
    auto& inFlightContext = context.InFlightContext();
    auto& deviceContext = context.DeviceContext();
    auto& descriptorLayoutCache = context.DescriptorLayoutCache();
    auto& descriptorAllocator = context.DescriptorAllocator();
    auto& pipelineLayoutCache = context.PipelineLayoutCache();
    auto [width, height] = context.Swapchain().Extent();

    m_sampler = CSampler { deviceContext, { .m_filtering = vk::Filter::eNearest } };

    // Attachments
    m_uiColor = InFlight<CImage> { inFlightContext, deviceContext, ImageCreateInfo {
        { width, height, 1 }, vk::Format::eR8G8B8A8Srgb, 1, 1,
        vk::SampleCountFlagBits::e1, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled
    }};

    // Descriptor sets
    const vk::raii::DescriptorSetLayout& uiSetLayout = descriptorLayoutCache.GetLayout({
        { 0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment },
        { 1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment }
    });

    m_uiDescriptorSet = InFlight { inFlightContext, descriptorAllocator.Allocate(std::vector(inFlightContext.FrameCount(), *uiSetLayout)) };

    // Write descriptors
    CDescriptorWriter descriptorWriter { deviceContext };
    for (auto i : Utils::Range(inFlightContext.FrameCount())) {
        descriptorWriter.Clear();
        descriptorWriter
            .WriteImage(
                0, inAttachment[i].View(),*m_sampler,
                vk::ImageLayout::eShaderReadOnlyOptimal, vk::DescriptorType::eCombinedImageSampler
            ).UpdateSet(*m_uiDescriptorSet[i]);
    }

    // Shaders
    const CShader vertexShaderUI(deviceContext, vk::ShaderStageFlagBits::eVertex, "res://shaders/shaderSwapchain.vert.spv");
    const CShader fragmentShaderUI(deviceContext, vk::ShaderStageFlagBits::eFragment, "res://shaders/shaderUI.frag.spv");

    const vk::raii::PipelineLayout& uiPipelineLayout = pipelineLayoutCache.GetLayout({
        { *uiSetLayout },
        { { vk::ShaderStageFlagBits::eFragment, 0, sizeof(UIConstants) } }
    });

    // Pipeline
    std::array uiColorFormats { m_uiColor.Get().Format() };
    m_pipelineUI = CGraphicsPipeline { deviceContext, {
        .m_layout = uiPipelineLayout,
        .m_shaders = { &vertexShaderUI, &fragmentShaderUI },
        .m_renderingInfo = { {}, uiColorFormats }
    }};
}

void CUIPass::WriteDescriptors(const CImage& texture) {
    CDescriptorWriter descriptorWriter { m_rendererContext->DeviceContext() };
    descriptorWriter.Clear();
    descriptorWriter
        .WriteImage(1, texture.View(), *m_sampler, vk::ImageLayout::eShaderReadOnlyOptimal, vk::DescriptorType::eCombinedImageSampler)
        .UpdateSet(*m_uiDescriptorSet.Get());
}

void CUIPass::Draw(const CCommandBuffer& cmd, const CImage& texture) {
    vk::RenderingAttachmentInfo uiAttachInfo {};
    uiAttachInfo.imageView = m_uiColor.Get().View();
    uiAttachInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
    uiAttachInfo.loadOp = vk::AttachmentLoadOp::eClear;
    uiAttachInfo.storeOp = vk::AttachmentStoreOp::eStore;
    uiAttachInfo.clearValue.color = std::array { 0.0f, 0.0f, 0.0f, 1.0f };

    vk::RenderingInfo uiRenderInfo {};
    uiRenderInfo.renderArea = vk::Rect2D { { 0, 0 }, m_uiColor.Get().Extent2D() };
    uiRenderInfo.layerCount = 1;
    uiRenderInfo.colorAttachmentCount = 1;
    uiRenderInfo.pColorAttachments = &uiAttachInfo;

    auto [width, height] = m_uiColor.Get().Extent2D();
    auto [textureWidth, textureHeight] = texture.Extent2D();

    cmd->beginRendering(uiRenderInfo);
    cmd->bindPipeline(vk::PipelineBindPoint::eGraphics, *m_pipelineUI);

    cmd->setViewport(0, {
        { 0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f }
    });
    cmd->setScissor(0, { { { 0, 0 }, { width, height } } });

    cmd->bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics, m_pipelineUI.Layout(),
        0, *m_uiDescriptorSet.Get(), {}
    );

    const UIConstants postProcessConstants = {
        glm::vec2(width, height),
        glm::vec2(textureWidth, textureHeight)
    };
    cmd->pushConstants<UIConstants>(
        m_pipelineUI.Layout(), vk::ShaderStageFlagBits::eFragment, 0, postProcessConstants
    );
    cmd->draw(3, 1, 0, 0);
    cmd->endRendering();
}

void CUIPass::Resize(const InFlight<CImage>& inAttachment) {
    const auto& deviceContext = m_rendererContext->DeviceContext();
    const auto& inFlightContext = m_rendererContext->InFlightContext();
    auto [width, height] = m_rendererContext->Swapchain().Extent();

    m_uiColor = InFlight<CImage> { inFlightContext, deviceContext, ImageCreateInfo {
        { width, height, 1 }, vk::Format::eR8G8B8A8Srgb, 1, 1,
        vk::SampleCountFlagBits::e1, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled
    }};

    CDescriptorWriter descriptorWriter { deviceContext };
    for (const auto i : Utils::Range(inFlightContext.FrameCount())) {
        descriptorWriter.Clear();
        descriptorWriter
            .WriteImage(0, inAttachment[i].View(), *m_sampler, vk::ImageLayout::eShaderReadOnlyOptimal, vk::DescriptorType::eCombinedImageSampler)
            .UpdateSet(*m_uiDescriptorSet[i]);
    }
}
}
