#include <skylabs/core/render/vulkan/main_pass.hpp>
#include <skylabs/core/render/vulkan/pipeline/descriptor_writer.hpp>

import glm;

namespace Vulkan {
struct MainConstants {
    alignas(16) std::uint32_t colorId = 0;
    alignas(16) glm::mat4x4 model = glm::gtc::identity<glm::mat4x4>();
};

CMainPass::CMainPass(const CreationTools& context, Utils::Extent2D renderExtent) :
    m_deviceContext(&context.m_deviceContext),
    m_inFlightContext(&context.m_inFlightContext)
{
    auto& inFlightContext = context.m_inFlightContext;
    auto& deviceContext = context.m_deviceContext;
    auto& descriptorLayoutCache = context.m_descriptorLayoutCache;
    auto& descriptorAllocator = context.m_descriptorAllocator;
    auto& pipelineLayoutCache = context.m_pipelineLayoutCache;
    auto [width, height] = renderExtent;

    m_nearestSampler = CSampler { deviceContext, { .m_filtering = vk::Filter::eNearest } };

    // Attachments
    m_mainColor = InFlight<CImage> { inFlightContext, deviceContext, ImageCreateInfo {
        { width, height, 1 }, vk::Format::eR8G8B8A8Srgb, 1, 1,
        vk::SampleCountFlagBits::e1, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled
    }};

    m_mainColorMSAA = InFlight<CImage> { inFlightContext, deviceContext, ImageCreateInfo {
        { width, height, 1 }, vk::Format::eR8G8B8A8Srgb, 1, 1,
        vk::SampleCountFlagBits::e4, vk::ImageUsageFlagBits::eColorAttachment
    }};

    m_mainDepthMSAA = InFlight<CImage> { inFlightContext, deviceContext, ImageCreateInfo {
        { width, height, 1 }, vk::Format::eD32Sfloat, 1, 1,
        vk::SampleCountFlagBits::e4, vk::ImageUsageFlagBits::eDepthStencilAttachment
    }};

    // Descriptors
    m_mvp = InFlight<CBuffer> { inFlightContext, deviceContext,
        sizeof(CMVP),
        vk::BufferUsageFlagBits::eUniformBuffer,
        MemoryLocation::eHostVisible
    };

    // Descriptor set
    const vk::raii::DescriptorSetLayout& mainSetLayout = descriptorLayoutCache.GetLayout({
        { 0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex },
    });

    m_mainDescriptorSet = InFlight { inFlightContext,
        descriptorAllocator.Allocate(std::vector(inFlightContext.FrameCount(), *mainSetLayout))
    };

    // Write descriptors
    CDescriptorWriter descriptorWriter { deviceContext };
    for (auto i : Utils::Range(inFlightContext.FrameCount())) {
        descriptorWriter.Clear();
        descriptorWriter
            .WriteBuffer(0, *m_mvp[i], m_mvp[i].Size(), 0, vk::DescriptorType::eUniformBuffer)
            .UpdateSet(*m_mainDescriptorSet[i]);
    }

    // Shaders
    const CShader vertexShader(deviceContext, vk::ShaderStageFlagBits::eVertex, "res://shaders/shader.vert.spv");
    const CShader fragmentShader(deviceContext, vk::ShaderStageFlagBits::eFragment, "res://shaders/shader.frag.spv");

    // Pipeline
    const vk::raii::PipelineLayout& mainPipelineLayout = pipelineLayoutCache.GetLayout({
        .m_descriptorSetLayouts = { *mainSetLayout },
        .m_pushConstants = {
            { vk::ShaderStageFlagBits::eFragment | vk::ShaderStageFlagBits::eVertex, 0, sizeof(MainConstants) }
        }
    });

    std::array colorFormats = { m_mainColor.Get().Format() };

    m_pipeline = CGraphicsPipeline { deviceContext, {
        .m_layout = *mainPipelineLayout,
        .m_shaders = { &vertexShader, &fragmentShader },
        .m_vertexBindings = {{
            .m_description = { 0, sizeof(CVertex) },
            .m_attributes = CVertex::GetAttributes() | std::ranges::to<std::vector>(),
        }},
        .m_renderingInfo = { {}, colorFormats, m_mainDepthMSAA.Get().Format() },
        .m_sampling =  vk::SampleCountFlagBits::e4
    }};
}

void CMainPass::WriteDescriptors(const std::vector<CImage>& textures) {
    CDescriptorWriter descriptorWriter { *m_deviceContext };
    for (const auto i : Utils::Range(m_inFlightContext->FrameCount())) {
        descriptorWriter.Clear();
        for (std::uint32_t j = 0; const auto& texture : textures) {
            descriptorWriter.WriteImage(
                1, texture.View(), *m_nearestSampler,
                vk::ImageLayout::eShaderReadOnlyOptimal, vk::DescriptorType::eCombinedImageSampler,j
            );
            j++;
        }
        descriptorWriter.UpdateSet(*m_mainDescriptorSet[i]);
    }
}

void CMainPass::Draw(
    const CCommandBuffer& cmd,
    const CBuffer& vertexBuffer,
    const CBuffer& indexBuffer,
    const std::span<const SubMesh> meshes,
    std::vector<CRenderObject> objects
) {
    vk::RenderingAttachmentInfo colorAttachInfo {};
    colorAttachInfo.imageView = m_mainColorMSAA.Get().View();
    colorAttachInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
    colorAttachInfo.loadOp = vk::AttachmentLoadOp::eClear;
    colorAttachInfo.storeOp = vk::AttachmentStoreOp::eStore;
    colorAttachInfo.clearValue.color = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);
    colorAttachInfo.resolveImageView = m_mainColor.Get().View();
    colorAttachInfo.resolveImageLayout = vk::ImageLayout::eColorAttachmentOptimal;
    colorAttachInfo.resolveMode = vk::ResolveModeFlagBits::eAverage;

    vk::RenderingAttachmentInfo depthAttachInfo {};
    depthAttachInfo.imageView = m_mainDepthMSAA.Get().View();
    depthAttachInfo.imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
    depthAttachInfo.loadOp = vk::AttachmentLoadOp::eClear;
    depthAttachInfo.storeOp = vk::AttachmentStoreOp::eDontCare;
    depthAttachInfo.clearValue.depthStencil = vk::ClearDepthStencilValue { 0.0f, 0 };

    vk::RenderingInfo mainRenderInfo {};
    mainRenderInfo.renderArea = vk::Rect2D { { 0, 0 }, m_mainColor.Get().Extent2D() };
    mainRenderInfo.layerCount = 1;
    mainRenderInfo.colorAttachmentCount = 1;
    mainRenderInfo.pColorAttachments = &colorAttachInfo;
    mainRenderInfo.pDepthAttachment = &depthAttachInfo;

    auto [width, height] = m_mainColor.Get().Extent2D();

    MainConstants mainConstants { };
    cmd->beginRendering(mainRenderInfo);
        cmd->bindPipeline(vk::PipelineBindPoint::eGraphics, *m_pipeline);

        cmd->setViewport(0, {
            { 0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f }
        });
        cmd->setScissor(0, { { { 0, 0 }, { width, height } } });

        cmd->bindVertexBuffers(0, { *vertexBuffer }, { 0 });
        cmd->bindIndexBuffer(*indexBuffer, 0, vk::IndexType::eUint16);

        cmd->bindDescriptorSets(
            vk::PipelineBindPoint::eGraphics, m_pipeline.Layout(),0,
        *m_mainDescriptorSet.Get(),{}
        );

        for(auto& object : objects) {
            mainConstants.colorId = object.colorId;
            mainConstants.model = object.model;
            auto& mesh = meshes[object.meshId];

            cmd->pushConstants<MainConstants>(
                m_pipeline.Layout(), vk::ShaderStageFlagBits::eFragment | vk::ShaderStageFlagBits::eVertex, 0, mainConstants
            );

            cmd->drawIndexed(
                mesh.indexCount, 1,
                static_cast<std::uint32_t>(mesh.IdxOffset() / 2),
                static_cast<std::int32_t>(mesh.VtxOffset() / sizeof(CVertex)),
                0
            );
        }
    cmd->endRendering();
}

void CMainPass::Resize(Utils::Extent2D newExtent) {
    auto [width, height] = newExtent;

    m_mainColor = InFlight<CImage> { *m_inFlightContext, *m_deviceContext, ImageCreateInfo {
        { width, height, 1 }, vk::Format::eR8G8B8A8Srgb, 1, 1,
        vk::SampleCountFlagBits::e1, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled
    }};

    m_mainColorMSAA = InFlight<CImage> { *m_inFlightContext, *m_deviceContext, ImageCreateInfo {
        { width, height, 1 }, vk::Format::eR8G8B8A8Srgb, 1, 1,
        vk::SampleCountFlagBits::e4, vk::ImageUsageFlagBits::eColorAttachment
    }};

    m_mainDepthMSAA = InFlight<CImage> { *m_inFlightContext, *m_deviceContext, ImageCreateInfo {
        { width, height, 1 }, vk::Format::eD32Sfloat, 1, 1,
        vk::SampleCountFlagBits::e4, vk::ImageUsageFlagBits::eDepthStencilAttachment
    }};
}
}
