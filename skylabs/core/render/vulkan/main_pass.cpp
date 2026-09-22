#include "skylabs/core/render/vulkan/main_pass.hpp"
#include "skylabs/core/render/vulkan/pipeline/descriptor_writer.hpp"

namespace {
struct MainConstants {
    alignas(16) std::uint32_t colorId = 0;
    alignas(16) glm::mat4x4 model = glm::identity<glm::mat4x4>();
};
}

namespace sk::render::vulkan {
MainPass::MainPass(const Device& device, const InFlightContext& inFlightContext,
                   const Allocator& allocator, PipelineLayoutCache& pipelineLayoutCache,
                   DescriptorLayoutCache& descriptorLayoutCache,
                   DescriptorAllocator& descriptorAllocator,
                   const filesystem::Filesystem& filesystem, utils::Extent2D renderExtent)
    : m_device(&device), m_allocator(&allocator), m_inFlightContext(&inFlightContext) {
    auto [width, height] = renderExtent;

    m_nearestSampler = Sampler { device, { .filtering = vk::Filter::eNearest } };

    // Attachments
    m_mainColor = InFlight<Image> { inFlightContext, *device, *allocator,
                                    ImageCreateInfo { { width, height, 1 },
                                                      vk::Format::eR8G8B8A8Srgb,
                                                      1,
                                                      1,
                                                      vk::SampleCountFlagBits::e1,
                                                      vk::ImageUsageFlagBits::eColorAttachment |
                                                          vk::ImageUsageFlagBits::eSampled } };

    m_mainColorMSAA = InFlight<Image> { inFlightContext, *device, *allocator,
                                        ImageCreateInfo { { width, height, 1 },
                                                          vk::Format::eR8G8B8A8Srgb,
                                                          1,
                                                          1,
                                                          vk::SampleCountFlagBits::e4,
                                                          vk::ImageUsageFlagBits::eColorAttachment } };

    m_mainDepthMSAA =
        InFlight<Image> { inFlightContext, *device, *allocator,
                          ImageCreateInfo { { width, height, 1 },
                                            vk::Format::eD32Sfloat,
                                            1,
                                            1,
                                            vk::SampleCountFlagBits::e4,
                                            vk::ImageUsageFlagBits::eDepthStencilAttachment } };

    // Descriptors
    m_mvp = InFlight<Buffer> { inFlightContext, *allocator, sizeof(MVP),
                               vk::BufferUsageFlagBits::eUniformBuffer, MemoryLocation::eHostVisible };

    // Descriptor set
    const vk::raii::DescriptorSetLayout& mainSetLayout = descriptorLayoutCache.GetLayout({
        { 0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex },
    });

    m_mainDescriptorSet =
        InFlight { inFlightContext, descriptorAllocator.Allocate(
                                        std::vector(inFlightContext.FrameCount(), *mainSetLayout)) };

    // Write descriptors
    DescriptorWriter descriptorWriter { *device };
    for (auto i : utils::Range(inFlightContext.FrameCount())) {
        descriptorWriter.Clear();
        descriptorWriter
            .WriteBuffer(0, *m_mvp[i], m_mvp[i].Size(), 0, vk::DescriptorType::eUniformBuffer)
            .UpdateSet(*m_mainDescriptorSet[i]);
    }

    // Shaders
    const Shader vertexShader(*device, vk::ShaderStageFlagBits::eVertex,
                              filesystem.LoadAsVector32("res://shaders/shader.vert.spv"));
    const Shader fragmentShader(*device, vk::ShaderStageFlagBits::eFragment,
                                filesystem.LoadAsVector32("res://shaders/shader.frag.spv"));

    // Pipeline
    const vk::raii::PipelineLayout& mainPipelineLayout = pipelineLayoutCache.GetLayout(
        { .descriptorSetLayouts = { *mainSetLayout },
          .pushConstants = { { vk::ShaderStageFlagBits::eFragment | vk::ShaderStageFlagBits::eVertex, 0,
                               sizeof(MainConstants) } } });

    std::array colorFormats = { m_mainColor.Get().Format() };

    m_pipeline =
        GraphicsPipeline { *device,
                           { .layout = *mainPipelineLayout,
                             .shaders = { &vertexShader, &fragmentShader },
                             .vertexBindings = { {
                                 .description = { 0, sizeof(Vertex) },
                                 .attributes = Vertex::GetAttributes() | std::ranges::to<std::vector>(),
                             } },
                             .renderingInfo = { { }, colorFormats, m_mainDepthMSAA.Get().Format() },
                             .sampling = vk::SampleCountFlagBits::e4 } };
}

void MainPass::WriteDescriptors(const std::vector<Image>& textures) {
    DescriptorWriter descriptorWriter { **m_device };
    for (const auto i : utils::Range(m_inFlightContext->FrameCount())) {
        descriptorWriter.Clear();
        for (std::uint32_t j = 0; const auto& texture : textures) {
            descriptorWriter.WriteImage(1, texture.View(), *m_nearestSampler,
                                        vk::ImageLayout::eShaderReadOnlyOptimal,
                                        vk::DescriptorType::eCombinedImageSampler, j);
            j++;
        }
        descriptorWriter.UpdateSet(*m_mainDescriptorSet[i]);
    }
}

void MainPass::Draw(const CommandBuffer& cmd, const Buffer& vertexBuffer, const Buffer& indexBuffer,
                    const std::span<const SubMesh> meshes, const std::vector<RenderObject>& objects) {
    auto& mainColor = m_mainColor.Get();

    vk::RenderingAttachmentInfo colorAttachInfo { };
    colorAttachInfo.imageView = m_mainColorMSAA.Get().View();
    colorAttachInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
    colorAttachInfo.loadOp = vk::AttachmentLoadOp::eClear;
    colorAttachInfo.storeOp = vk::AttachmentStoreOp::eStore;
    colorAttachInfo.clearValue.color = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 0.0f);
    colorAttachInfo.resolveImageView = mainColor.View();
    colorAttachInfo.resolveImageLayout = vk::ImageLayout::eColorAttachmentOptimal;
    colorAttachInfo.resolveMode = vk::ResolveModeFlagBits::eAverage;

    vk::RenderingAttachmentInfo depthAttachInfo { };
    depthAttachInfo.imageView = m_mainDepthMSAA.Get().View();
    depthAttachInfo.imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
    depthAttachInfo.loadOp = vk::AttachmentLoadOp::eClear;
    depthAttachInfo.storeOp = vk::AttachmentStoreOp::eDontCare;
    depthAttachInfo.clearValue.depthStencil = vk::ClearDepthStencilValue { 0.0f, 0 };

    vk::RenderingInfo mainRenderInfo { };
    mainRenderInfo.renderArea = vk::Rect2D { { 0, 0 }, mainColor.Extent2D() };
    mainRenderInfo.layerCount = 1;
    mainRenderInfo.colorAttachmentCount = 1;
    mainRenderInfo.pColorAttachments = &colorAttachInfo;
    mainRenderInfo.pDepthAttachment = &depthAttachInfo;

    auto [width, height] = mainColor.Extent2D();

    MainConstants mainConstants { };
    cmd->beginRendering(mainRenderInfo);
    cmd->bindPipeline(vk::PipelineBindPoint::eGraphics, *m_pipeline);

    cmd->setViewport(
        0, { { 0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f } });
    cmd->setScissor(0, { { { 0, 0 }, { width, height } } });

    cmd->bindVertexBuffers(0, { *vertexBuffer }, { 0 });
    cmd->bindIndexBuffer(*indexBuffer, 0, vk::IndexType::eUint16);

    cmd->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_pipeline.Layout(), 0,
                            *m_mainDescriptorSet.Get(), { });

    for (auto& object : objects) {
        mainConstants.colorId = object.colorId;
        mainConstants.model = object.model;
        auto& mesh = meshes[object.meshId];

        cmd->pushConstants<MainConstants>(
            m_pipeline.Layout(), vk::ShaderStageFlagBits::eFragment | vk::ShaderStageFlagBits::eVertex,
            0, mainConstants);

        cmd->drawIndexed(mesh.indexCount, 1, static_cast<std::uint32_t>(mesh.IdxOffset() / 2),
                         static_cast<std::int32_t>(mesh.VtxOffset() / sizeof(Vertex)), 0);
    }
    cmd->endRendering();
}

void MainPass::Resize(utils::Extent2D newExtent) {
    auto [width, height] = newExtent;

    m_mainColor = InFlight<Image> { *m_inFlightContext, **m_device, **m_allocator,
                                    ImageCreateInfo { { width, height, 1 },
                                                      vk::Format::eR8G8B8A8Srgb,
                                                      1,
                                                      1,
                                                      vk::SampleCountFlagBits::e1,
                                                      vk::ImageUsageFlagBits::eColorAttachment |
                                                          vk::ImageUsageFlagBits::eSampled } };

    m_mainColorMSAA = InFlight<Image> { *m_inFlightContext, **m_device, **m_allocator,
                                        ImageCreateInfo { { width, height, 1 },
                                                          vk::Format::eR8G8B8A8Srgb,
                                                          1,
                                                          1,
                                                          vk::SampleCountFlagBits::e4,
                                                          vk::ImageUsageFlagBits::eColorAttachment } };

    m_mainDepthMSAA =
        InFlight<Image> { *m_inFlightContext, **m_device, **m_allocator,
                          ImageCreateInfo { { width, height, 1 },
                                            vk::Format::eD32Sfloat,
                                            1,
                                            1,
                                            vk::SampleCountFlagBits::e4,
                                            vk::ImageUsageFlagBits::eDepthStencilAttachment } };
}
}
