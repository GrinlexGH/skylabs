#include <skylabs/core/render/vulkan/renderer.hpp>
#include <skylabs/public/logging.hpp>
#include <skylabs/public/sdl/filesystem.hpp>
#include <skylabs/core/render/vulkan/pipeline/shader.hpp>
#include <skylabs/core/render/vulkan/render_graph/graph.hpp>
#include <skylabs/core/render/vulkan/render_graph/descriptor_writer.hpp>
#include <skylabs/core/camera.hpp>

#include <boost/container_hash/hash.hpp>
#include <glm/gtx/hash.hpp>
#include <glm/ext/scalar_reciprocal.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <SDL3_image/SDL_image.h>
#include <tiny_obj_loader.h>

#include <chrono>
#include <random>
#include <ranges>

template<> struct std::hash<CVertex> {
    size_t operator()(const CVertex& vertex) const noexcept {
        std::size_t seed = 0;
        boost::hash_combine(seed, vertex.m_position.x);
        boost::hash_combine(seed, vertex.m_position.y);
        boost::hash_combine(seed, vertex.m_position.z);

        boost::hash_combine(seed, vertex.m_texCoord.x);
        boost::hash_combine(seed, vertex.m_texCoord.y);
        return seed;
    }
};

struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

struct PushConstants {
    std::uint32_t textureIndex = 0;
};

namespace {
std::uint32_t renderWidth = 0;
std::uint32_t renderHeight = 0;

vk::raii::CommandBuffer BeginSingleTimeCommands(
    const vk::raii::Device& device,
    const vk::CommandPool& commandPool
) {
    vk::CommandBufferAllocateInfo allocInfo {};
    allocInfo.level = vk::CommandBufferLevel::ePrimary;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    vk::raii::CommandBuffers commandBuffers { device, allocInfo };
    commandBuffers[0].begin({ vk::CommandBufferUsageFlagBits::eOneTimeSubmit });

    return vk::raii::CommandBuffer { std::move(commandBuffers[0]) };
}

void EndSingleTimeCommands(
    const Vulkan::CDevice& device,
    vk::raii::CommandBuffer& commandBuffer
) {
    commandBuffer.end();

    vk::SubmitInfo submitInfo {};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &*commandBuffer;

    vk::raii::Fence fence { *device, vk::FenceCreateInfo { } };

    device.GraphicsQueue()->submit(submitInfo, fence );

    vk::Result result = device->waitForFences({ fence }, vk::True, std::numeric_limits<std::uint64_t>::max());
    if (result != vk::Result::eSuccess) {
        throw std::runtime_error("Failed to wait for fence: " + vk::to_string(result));
    }

    commandBuffer.clear();
}

glm::mat4 ReverseZPerspective(const unsigned int width, const unsigned int height, const float fov = 90, const float nearZ = 0.01f) {
    glm::mat4 proj;
    float g = 1.0f / std::tan(0.5f * glm::radians(fov));
    proj = glm::mat4(0.0f);
    proj[0][0] = g / (static_cast<float>(width) / static_cast<float>(height));
    proj[1][1] = -g;
    proj[2][3] = -1.0f;
    proj[3][2] = nearZ;

    return proj;
}

void UpdateUniformBuffer(
    const vk::Extent2D& cameraDimensions,
    Vulkan::CBuffer& uniformBuffer,
    const glm::mat4& view,
    const float fov,
    const vk::SurfaceTransformFlagBitsKHR transform
) {
    glm::mat4 rot = glm::mat4(1.0f);

    if (transform == vk::SurfaceTransformFlagBitsKHR::eRotate90) {
        rot = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0, 0, 1));
    } else if (transform == vk::SurfaceTransformFlagBitsKHR::eRotate270) {
        rot = glm::rotate(glm::mat4(1.0f), glm::radians(270.0f), glm::vec3(0, 0, 1));
    } else if (transform == vk::SurfaceTransformFlagBitsKHR::eRotate180) {
        rot = glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0, 0, 1));
    }

    UniformBufferObject ubo {
        .model = glm::mat4(1.0f),
        .view = view,
        .proj = rot * ReverseZPerspective(cameraDimensions.width, cameraDimensions.height, fov),
    };

    std::memcpy(uniformBuffer.Data(), &ubo, sizeof(ubo));
}
}

namespace Vulkan {
CRenderer::CRenderer(const IWindow* const window) {
    assert(window);

    m_context = CContext { window };

    m_swapchain = CSwapchain { m_context, *m_context.Surface(), 2, vk::PresentModeKHR::eMailbox };
    renderWidth = m_swapchain.Extent().width;
    renderHeight = m_swapchain.Extent().height;

    m_graphicsCommands = CCommandBufferSet { m_context, m_context.Device().GraphicsQueue().FamilyIndex(), { FRAMES_IN_FLIGHT_COUNT, 0 } };
    m_computeCommands = CCommandBufferSet { m_context, m_context.Device().ComputeQueue().FamilyIndex(), { 0, 0 } };

    m_pipelineLayoutCache = CPipelineLayoutCache { m_context };
    m_descriptorLayoutCache = CDescriptorLayoutCache { m_context };

    m_descriptorAllocator = CDescriptorAllocator { m_context };

    m_inFlightContext = InFlightContext { FRAMES_IN_FLIGHT_COUNT };
    m_firstUse = InFlight<bool> { m_inFlightContext, true };

    m_fence = InFlight<vk::raii::Fence> { m_inFlightContext, *m_context.Device(), vk::FenceCreateInfo { vk::FenceCreateFlagBits::eSignaled } };
    m_isRenderFinishedSemaphore = InFlight<vk::raii::Semaphore> { m_inFlightContext, *m_context.Device(), vk::SemaphoreCreateInfo {} };

    m_mainColor = InFlight<CImage> { m_inFlightContext, m_context, ImageCreateInfo {
        { renderWidth, renderHeight, 1 }, vk::Format::eR8G8B8A8Srgb, 1, 1,
        vk::SampleCountFlagBits::e1, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled
    }};

    m_mainColorMSAA = InFlight<CImage> { m_inFlightContext, m_context, ImageCreateInfo {
        { renderWidth, renderHeight, 1 }, vk::Format::eR8G8B8A8Srgb, 1, 1,
        vk::SampleCountFlagBits::e4, vk::ImageUsageFlagBits::eColorAttachment
    }};

    m_mainDepthMSAA = InFlight<CImage> { m_inFlightContext, m_context, ImageCreateInfo {
        { renderWidth, renderHeight, 1 }, vk::Format::eD32Sfloat, 1, 1,
        vk::SampleCountFlagBits::e4, vk::ImageUsageFlagBits::eDepthStencilAttachment
    }};

    m_uniform = InFlight<CBuffer> { m_inFlightContext, m_context,
        sizeof(UniformBufferObject),
        vk::BufferUsageFlagBits::eUniformBuffer,
        MemoryLocation::eHostVisible
    };

    const vk::raii::DescriptorSetLayout& mainSetLayout = m_descriptorLayoutCache.GetLayout({
        { 0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex },
        { 1, vk::DescriptorType::eCombinedImageSampler, 1024, vk::ShaderStageFlagBits::eFragment }
    });

    const vk::raii::DescriptorSetLayout& swapchainSetLayout = m_descriptorLayoutCache.GetLayout({
        { 0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment }
    });

    m_descriptorAllocator.Allocate(std::vector());

    const std::uint32_t imageCount = m_swapchain.Images().size();
    m_renderFinishedSemaphores.reserve(imageCount);
    for (std::size_t i = 0; i < imageCount; ++i) {
        m_renderFinishedSemaphores.emplace_back(*m_context.Device(), vk::SemaphoreCreateInfo {});
    }

    CBuffer stagingBuffer { nullptr };
    m_singleCommandPool = m_context.Device()->createCommandPool({
        vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        m_context.Device().GraphicsQueue().FamilyIndex()
    });

    m_mainSampler = CSampler { m_context };
    m_modelTextureSampler = CSampler { m_context };

    m_textureManager = CTexturePool { m_context, m_swapchain.Extent(), FRAMES_IN_FLIGHT_COUNT };
    auto& txm = m_textureManager;

    m_colorBuffer = txm.CreateTexture("colorBuffer", {
        .m_extent = RelativeTextureSize {},
        .m_usage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
    });

    m_colorBufferMSAAx = txm.CreateTexture("colorBufferMSAAx", {
        .m_extent = RelativeTextureSize {},
        .m_usage = vk::ImageUsageFlagBits::eColorAttachment,
        .m_sampleCount = vk::SampleCountFlagBits::e4,
    });

    m_depthBufferMSAAx = txm.CreateTexture("depthBufferMSAAx", {
        .m_extent = RelativeTextureSize {},
        .m_format = vk::Format::eD32Sfloat,
        .m_usage = vk::ImageUsageFlagBits::eDepthStencilAttachment,
        .m_sampleCount = vk::SampleCountFlagBits::e4
    });

    LoadModelTextures(stagingBuffer, m_singleCommandPool);

    txm.GenerateTextures();


    m_bufferManager = CBufferPool { m_context, FRAMES_IN_FLIGHT_COUNT };
    auto& bfm = m_bufferManager;

    m_uniformBuffer = bfm.CreateBuffer("global-uniform", {
        .m_size = sizeof(UniformBufferObject),
        .m_location = MemoryLocation::eHostVisible,
        .m_usage = vk::BufferUsageFlagBits::eUniformBuffer,
        .m_isInFlight = true,
    });

    LoadModels(stagingBuffer, m_singleCommandPool);

    bfm.GenerateBuffers();

    m_descriptorManager = CDescriptorPool { m_context, FRAMES_IN_FLIGHT_COUNT };
    auto& dsm = m_descriptorManager;

    m_mainDescriptorSet = dsm.CreateDescriptorSet({
        {
            .m_binding = 0,
            .m_type = vk::DescriptorType::eUniformBuffer,
            .m_shaderStages = vk::ShaderStageFlagBits::eVertex,
            .m_info = BufferDescriptorInfo { .m_buffer = m_uniformBuffer }
        },
        {
            .m_binding = 1,
            .m_type = vk::DescriptorType::eCombinedImageSampler,
            .m_shaderStages = vk::ShaderStageFlagBits::eFragment,
            .m_count = 1024,
        }
    });

    m_swapchainDescriptorSet = dsm.CreateDescriptorSet({
        {
            .m_binding = 0,
            .m_type = vk::DescriptorType::eCombinedImageSampler,
            .m_shaderStages = vk::ShaderStageFlagBits::eFragment,
            .m_info = SampledImageDescriptorInfo { .m_image = m_colorBuffer, .m_sampler = *m_mainSampler }
        }
    });

    dsm.CreateDescriptorPool();
    dsm.CreateDescriptorSets();
    dsm.UpdateDescriptorSets(m_bufferManager, m_textureManager);
    dsm.BindTextureToIndex(m_mainDescriptorSet, 1, 0, m_textureManager.GetTexture(m_matroskinModelTexture), *m_modelTextureSampler);
    dsm.BindTextureToIndex(m_mainDescriptorSet, 1, 1, m_textureManager.GetTexture(m_roomModelTexture), *m_modelTextureSampler);

    // Main pipeline
    // Shaders
    const CShader vertexShader(m_context, vk::ShaderStageFlagBits::eVertex, "res://shaders/shader.vert.spv");
    const CShader fragmentShader(m_context, vk::ShaderStageFlagBits::eFragment, "res://shaders/shader.frag.spv");

    // Pipeline
    const vk::raii::PipelineLayout& mainPipelineLayout = m_pipelineLayoutCache.GetLayout({
        { *dsm.GetDescriptorSetLayout(m_mainDescriptorSet) }, { { vk::ShaderStageFlagBits::eFragment, 0, sizeof(std::uint32_t) } }
    });

    std::array<vk::Format, 1> colorFormats = { txm.GetTexture(m_colorBuffer).Format() };

    m_pipelineMain = CGraphicsPipeline { m_context, {
        .m_layout = *mainPipelineLayout,
        .m_shaders = { &vertexShader, &fragmentShader },
        .m_vertexBindings = {{
            .m_description = { 0, sizeof(CVertex) },
            .m_attributes = CVertex::GetAttributes() | std::ranges::to<std::vector>(),
        }},
        .m_renderingInfo = { {}, colorFormats, txm.GetTexture(m_depthBufferMSAAx).Format() },
        .m_sampling =  vk::SampleCountFlagBits::e4
    }
    };

    // Swapchain pipeline
    // Shaders
    const CShader vertexShaderSwapchain(m_context, vk::ShaderStageFlagBits::eVertex, "res://shaders/shaderSwapchain.vert.spv");
    const CShader fragmentShaderSwapchain(m_context, vk::ShaderStageFlagBits::eFragment, "res://shaders/shaderSwapchain.frag.spv");

    const vk::raii::PipelineLayout& swapchainPipelineLayout = m_pipelineLayoutCache.GetLayout({
        { *dsm.GetDescriptorSetLayout(m_swapchainDescriptorSet) }
    });

    // Pipeline
    std::array<vk::Format, 1> swapchainColorFormats { m_swapchain.SurfaceFormat().format };
    m_pipelineSwapchain = CGraphicsPipeline { m_context, {
        .m_layout = swapchainPipelineLayout,
        .m_shaders = { &vertexShaderSwapchain, &fragmentShaderSwapchain },
        .m_renderingInfo = { {}, swapchainColorFormats }
    }};
}

CRenderer::~CRenderer() {
    if (**m_context.Device()) {
        try {
            m_context.Device()->waitIdle();
            for (auto& b : m_context.Allocator()->getHeapBudgets()) {
                Log::Debug("My heap currently has {} allocations taking {} B,",
                    b.statistics.allocationCount,
                    b.statistics.allocationBytes
                );
                Log::Debug("allocated out of {} Vulkan device memory blocks taking {} B,",
                    b.statistics.blockCount,
                    b.statistics.blockBytes
                );
                Log::Debug("Vulkan reports total usage {} B with budget {} B.\n",
                    b.usage,
                    b.budget
                );
            }
        } catch (const vk::SystemError& e) {
            Log::Error("Failed to wait device idle in renderer destructor: {}", e.what());
        }
    }
}

std::unique_ptr<CRenderer> CRenderer::TryToCreate(const IWindow* const window) {
    try {
        return std::make_unique<CRenderer>(window);
    } catch (const std::exception& e) {
        Log::Error("Cannot initialize vulkan renderer: {}", e.what());
        return nullptr;
    }
}

void CRenderer::Draw(const glm::mat4 view, const float fov, float) {
    m_textureManager.SetFrameIndex(m_frameContext.m_frameIndex);
    m_bufferManager.SetFrameIndex(m_frameContext.m_frameIndex);
    m_descriptorManager.SetFrameIndex(m_frameContext.m_frameIndex);
    auto& cmd = m_graphicsCommands.PrimaryBuffers()[m_frameContext.m_frameIndex];
    CFrame& frameData = m_frameData[m_frameContext.m_frameIndex];
    const CDevice& device = m_context.Device();
    auto& uniformBuffer = m_bufferManager.GetBuffer(m_uniformBuffer);
    auto& vertexBuffer = m_bufferManager.GetBuffer(m_vertexBuffer);
    auto& indexBuffer = m_bufferManager.GetBuffer(m_indexBuffer);
    auto& colorBuffer = m_textureManager.GetTexture(m_colorBuffer);
    auto& colorBufferMSAA = m_textureManager.GetTexture(m_colorBufferMSAAx);
    auto& depthBufferMSAA = m_textureManager.GetTexture(m_depthBufferMSAAx);
    auto descriptorSetMain = m_descriptorManager.GetDescriptorSet(m_mainDescriptorSet);
    auto descriptorSetSwapchain = m_descriptorManager.GetDescriptorSet(m_swapchainDescriptorSet);

    if (m_isResized) {
        Resize(frameData);
    }

    UpdateUniformBuffer(m_swapchain.Extent(), uniformBuffer, view, fov, m_swapchain.SurfaceTransform());

    // Wait for fence to ensure that the previous frame rendering is finished
    vk::Result result = device->waitForFences({ frameData.GetFence() }, vk::True, std::numeric_limits<std::uint64_t>::max());
    if (result != vk::Result::eSuccess) {
        throw std::runtime_error("Failed to wait for fence: " + vk::to_string(result));
    }

    // Acquire next image from the swapchain
    auto acquireResult = m_swapchain.AcquireImage(*frameData.GetImageAvailableSemaphore());
    if (!acquireResult) {
        vk::Result error = acquireResult.error();
        if (error == vk::Result::eErrorSurfaceLostKHR) {
            m_swapchain.Clear();
            m_context.RecreateSurface();
            Resize(frameData);
            return;
        }

#ifdef PLATFORM_ANDROID
        if (error == vk::Result::eSuboptimalKHR) {
            return;
        }
#endif

        if (error == vk::Result::eErrorOutOfDateKHR || error == vk::Result::eSuboptimalKHR) {
            Resize(frameData);
            return;
        }

        throw std::runtime_error(fmt::format("Failed to present image: {}", vk::to_string(error)));
    }

    std::uint32_t imageIndex = *acquireResult;

    // Reset fence after resizing to avoid deadlock on next invocation of Draw()
    device->resetFences({ frameData.GetFence() });

    cmd->reset();
    cmd->begin({});

    if (m_firstUse[m_frameContext.m_frameIndex]) {
        cmd.PipelineBarrier({
            ImageBarrier { colorBuffer, colorBuffer.FullRange(), Usage::eNone, Usage::eColorAttachment },
            ImageBarrier { colorBufferMSAA, colorBufferMSAA.FullRange(), Usage::eNone, Usage::eColorAttachment },
            ImageBarrier { depthBufferMSAA, depthBufferMSAA.FullRange(), Usage::eNone, Usage::eDepthWrite },
        });
        m_firstUse[m_frameContext.m_frameIndex] = false;
    } else {
        cmd.PipelineBarrier({
            ImageBarrier { colorBuffer, colorBuffer.FullRange(), Usage::eSampledFragment, Usage::eColorAttachment },
        });
    }

    // Main render
    vk::RenderingAttachmentInfo colorAttachInfo {};
    colorAttachInfo.imageView = colorBufferMSAA.View();
    colorAttachInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
    colorAttachInfo.loadOp = vk::AttachmentLoadOp::eClear;
    colorAttachInfo.storeOp = vk::AttachmentStoreOp::eStore;
    colorAttachInfo.clearValue.color = vk::ClearColorValue(11.0f / 255.0f, 16.0f / 255.0f, 38.0f / 255.0f, 1.0f);
    colorAttachInfo.resolveImageView = colorBuffer.View();
    colorAttachInfo.resolveImageLayout = vk::ImageLayout::eColorAttachmentOptimal;
    colorAttachInfo.resolveMode = vk::ResolveModeFlagBits::eAverage;

    vk::RenderingAttachmentInfo depthAttachInfo {};
    depthAttachInfo.imageView = depthBufferMSAA.View();
    depthAttachInfo.imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
    depthAttachInfo.loadOp = vk::AttachmentLoadOp::eClear;
    depthAttachInfo.storeOp = vk::AttachmentStoreOp::eDontCare;
    depthAttachInfo.clearValue.depthStencil = vk::ClearDepthStencilValue { 0.0f, 0 };

    vk::RenderingInfo mainRenderInfo {};
    mainRenderInfo.renderArea = vk::Rect2D { { 0, 0 }, colorBuffer.Extent2D() };
    mainRenderInfo.layerCount = 1;
    mainRenderInfo.colorAttachmentCount = 1;
    mainRenderInfo.pColorAttachments = &colorAttachInfo;
    mainRenderInfo.pDepthAttachment = &depthAttachInfo;

    PushConstants constants { 0 };

    cmd->beginRendering(mainRenderInfo);
        cmd->bindPipeline(vk::PipelineBindPoint::eGraphics, *m_pipelineMain);

        cmd->setViewport(0, { { 0.0f, 0.0f, static_cast<float>(colorBuffer.Extent().width), static_cast<float>(colorBuffer.Extent().height), 0.0f, 1.0f } });
        cmd->setScissor(0, { { { 0, 0 }, colorBuffer.Extent2D() } });
        cmd->setDepthBiasEnable(vk::False);

        cmd->bindVertexBuffers(0, { *vertexBuffer }, { 0 });
        cmd->bindIndexBuffer(*indexBuffer, 0, vk::IndexType::eUint16);

        cmd->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_pipelineMain.Layout(), 0, descriptorSetMain, {});

        constants = { 0 };
        cmd->pushConstants<PushConstants>(m_pipelineMain.Layout(), vk::ShaderStageFlagBits::eFragment, 0, constants);
        cmd->drawIndexed(m_matroskin.indexCount, 1, m_matroskin.IdxOffset() / 2, static_cast<int32_t>(m_matroskin.VtxOffset() / sizeof(CVertex)), 0);

        constants = { 1 };
        cmd->pushConstants<PushConstants>(m_pipelineMain.Layout(), vk::ShaderStageFlagBits::eFragment, 0, constants);
        cmd->drawIndexed(m_viking.indexCount, 1, m_viking.IdxOffset() / 2, static_cast<int32_t>( m_viking.VtxOffset() / sizeof(CVertex)), 0);
    cmd->endRendering();

    cmd.PipelineBarrier({
        ImageBarrier { colorBuffer, colorBuffer.FullRange(), Usage::eColorAttachment, Usage::eSampledFragment },
        ImageBarrier { m_swapchain.Images()[imageIndex], m_swapchain.Images()[imageIndex].FullRange(), Usage::eNone, Usage::eColorAttachment }
    });

    // Fullscreen triangle
    vk::RenderingAttachmentInfo swapchainAttachInfo {};
    swapchainAttachInfo.imageView = *m_swapchain.Images()[imageIndex].View();
    swapchainAttachInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
    swapchainAttachInfo.loadOp = vk::AttachmentLoadOp::eClear;
    swapchainAttachInfo.storeOp = vk::AttachmentStoreOp::eStore;
    swapchainAttachInfo.clearValue.color = std::array { 0.0f, 0.0f, 0.0f, 1.0f };

    vk::RenderingInfo swapchainRenderInfo {};
    swapchainRenderInfo.renderArea = vk::Rect2D { { 0, 0 }, m_swapchain.Extent() };
    swapchainRenderInfo.layerCount = 1;
    swapchainRenderInfo.colorAttachmentCount = 1;
    swapchainRenderInfo.pColorAttachments = &swapchainAttachInfo;

    cmd->beginRendering(swapchainRenderInfo);
        cmd->bindPipeline(vk::PipelineBindPoint::eGraphics, *m_pipelineSwapchain);

        cmd->setViewport(0, { { 0.0f, 0.0f, static_cast<float>(m_swapchain.Extent().width), static_cast<float>(m_swapchain.Extent().height), 0.0f, 1.0f } });
        cmd->setScissor(0, { { { 0, 0 }, m_swapchain.Extent() } });
        cmd->setDepthBiasEnable(vk::False);

        cmd->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_pipelineSwapchain.Layout(), 0, descriptorSetSwapchain, {});
        cmd->draw(3, 1, 0, 0);
    cmd->endRendering();

    cmd.PipelineBarrier({
        ImageBarrier { m_swapchain.Images()[imageIndex], m_swapchain.Images()[imageIndex].FullRange(), Usage::eColorAttachment, Usage::ePresent }
    });

    cmd->end();

    vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;

    vk::SubmitInfo finalSubmit {};
    finalSubmit.setWaitSemaphores({ *frameData.GetImageAvailableSemaphore() });
    finalSubmit.setWaitDstStageMask({ waitStage });
    finalSubmit.setCommandBuffers({ **cmd });
    finalSubmit.setSignalSemaphores({ *m_renderFinishedSemaphores[imageIndex] });
    m_context.Device().GraphicsQueue()->submit(finalSubmit, frameData.GetFence());

    // Present
    auto presentResult = m_swapchain.PresentImage(imageIndex, { *m_renderFinishedSemaphores[imageIndex] });
    if (presentResult != vk::Result::eSuccess) {
        if (presentResult == vk::Result::eErrorSurfaceLostKHR) {
            m_swapchain.Clear();
            m_context.RecreateSurface();
            Resize(frameData);
            return;
        }

#ifdef PLATFORM_ANDROID
        if (presentResult == vk::Result::eSuboptimalKHR) {
            return;
        }
#endif

        if (presentResult == vk::Result::eErrorOutOfDateKHR || presentResult == vk::Result::eSuboptimalKHR) {
            Resize(frameData);
            return;
        }

        throw std::runtime_error(fmt::format("Failed to present image: {}", vk::to_string(presentResult)));
    }

    m_frameContext.m_frameIndex = (m_frameContext.m_frameIndex + 1) % FRAMES_IN_FLIGHT_COUNT;
}

void CRenderer::Resize(CFrame& currentFrameData) {
    for (auto&& i : m_firstUse) { i = true; }

    for (auto& frame : m_frameData) {
        vk::Result result = m_context.Device()->waitForFences({ frame.GetFence() }, vk::True, std::numeric_limits<std::uint64_t>::max());
        if (result != vk::Result::eSuccess) {
            throw std::runtime_error("Failed to wait for fence: " + vk::to_string(result));
        }
    }

    auto [width, height] = m_context.Window()->DrawableSize();
    renderWidth = width;
    renderHeight = height;

    m_swapchain.Recreate(*m_context.Surface());

    m_textureManager.Resize(m_swapchain.Extent());
    m_descriptorManager.UpdateDescriptorSets(m_bufferManager, m_textureManager);

    currentFrameData.RecreateImageAvailableSemaphore();
    m_isResized = false;
}

void CRenderer::LoadModelTextures(CBuffer& stagingBuffer, const vk::raii::CommandPool& commandPool) {
    auto UploadTexture = [&](const std::string& path, const std::string& debugName) -> TextureHandle {
        std::unique_ptr<IFileStream> stream = Filesystem::LoadAsIO(path);
        SDL_IOStream* sdlStream = SDL::CreateIOStreamFromResource(stream.get());

        SDL_Surface* imageRaw = IMG_Load_IO(sdlStream, false);
        if (!imageRaw) {
            throw std::runtime_error("Failed to load texture: " + path);
        }

        SDL_Surface* image = SDL_ConvertSurface(imageRaw, SDL_PIXELFORMAT_RGBA32);
        SDL_DestroySurface(imageRaw);

        const vk::DeviceSize imageSize = static_cast<vk::DeviceSize>(image->w) * image->h * 4;
        if (stagingBuffer.Size() < imageSize) {
            stagingBuffer = CBuffer {
                m_context, imageSize,
                vk::BufferUsageFlagBits::eTransferSrc,
                MemoryLocation::eHostVisible
            };
        }
        std::memcpy(stagingBuffer.Data(), image->pixels, static_cast<std::size_t>(imageSize));

        std::uint32_t mipLevels = static_cast<std::uint32_t>(std::floor(std::log2(std::max(image->w, image->h)))) + 1;

        CImage texture { m_context, {
            .m_extent = vk::Extent3D { static_cast<std::uint32_t>(image->w), static_cast<std::uint32_t>(image->h), 1 },
            .m_format = vk::Format::eR8G8B8A8Srgb,
            .m_mipLevels = mipLevels,
            .m_usageFlags = vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
        }};

        {
            vk::raii::CommandBuffer commandBuffer = BeginSingleTimeCommands(*m_context.Device(), commandPool);
            CCommandBuffer cmd { commandBuffer };

            cmd.PipelineBarrier({ ImageBarrier { texture, texture.FullRange(), Vulkan::Usage::eNone, Vulkan::Usage::eTransferWrite } });
            cmd.Copy(texture, stagingBuffer);

            const vk::FormatProperties formatProperties = m_context.PhysicalDevice()->getFormatProperties(texture.Format());
            if (!(formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear)) {
                throw std::runtime_error("Texture format does not support linear blitting for: " + path);
            }

            cmd.GenerateMipmaps(texture);

            EndSingleTimeCommands(m_context.Device(), commandBuffer);
        }

        SDL_DestroySurface(image);

        return m_textureManager.ImportTexture(debugName.c_str(), std::move(texture));
    };

    m_roomModelTexture = UploadTexture("assets://viking_room.png", "viking-room");
    m_matroskinModelTexture = UploadTexture("assets://matroskin.png", "matroskin");
}

auto LoadModel(const char* filename) {
    std::vector<CVertex> vertices;
    std::vector<std::uint16_t> indices;

    tinyobj::ObjReader reader;
    reader.ParseFromString(Filesystem::LoadAsString(filename), "");
    if (!reader.Valid()) {
        throw std::runtime_error(reader.Warning() + " " + reader.Error());
    }

    tinyobj::attrib_t attrib = reader.GetAttrib();
    std::vector<tinyobj::shape_t> shapes = reader.GetShapes();

    std::unordered_map<CVertex, uint32_t> uniqueVertices {};
    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            CVertex vertex {};

            if (index.vertex_index >= 0) {
                vertex.m_position = {
                    attrib.vertices[(3 * index.vertex_index) + 0],
                    attrib.vertices[(3 * index.vertex_index) + 1],
                    attrib.vertices[(3 * index.vertex_index) + 2]
                };
            }

            if (index.texcoord_index >= 0) {
                vertex.m_texCoord = {
                    attrib.texcoords[(2 * index.texcoord_index) + 0],
                    1.0f - attrib.texcoords[(2 * index.texcoord_index) + 1]
                };
            } else {
                vertex.m_texCoord = {
                    (vertex.m_position.x * 0.5f) + 0.5f,
                    (vertex.m_position.z * 0.5f) + 0.5f
                };
            }

            if (!uniqueVertices.contains(vertex)) {
                uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                vertices.push_back(vertex);
            }

            indices.push_back(static_cast<uint16_t> (uniqueVertices[vertex]));
        }
    }

    return std::tuple { vertices, indices };
}

void CRenderer::LoadModels(CBuffer& stagingBuffer, const vk::raii::CommandPool& commandPool) {
    m_vertexBuffer = m_bufferManager.ImportBuffer("vertexBuffer", CBuffer {
        m_context, GEOMETRY_POOL_SIZE,
        vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
        MemoryLocation::eDeviceOnly
    });
    m_indexBuffer = m_bufferManager.ImportBuffer("indexBuffer", CBuffer {
        m_context, GEOMETRY_POOL_SIZE,
        vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
        MemoryLocation::eDeviceOnly
    });

    auto UploadToPool = [&](const std::string& path, SubMesh& subMesh) {
        auto [vertices, indices] = LoadModel(path.c_str());

        vk::DeviceSize vSize = vertices.size() * sizeof(vertices[0]);
        vk::DeviceSize iSize = indices.size() * sizeof(indices[0]);

        vma::VirtualAllocationCreateInfo allocationInfo { };
        allocationInfo.setSize(vSize);
        subMesh.vtxAlloc = vma::raii::VirtualAllocation { m_bufferManager.GetBuffer(m_vertexBuffer).VirtualBlock(), allocationInfo };
        allocationInfo.setSize(iSize);
        subMesh.idxAlloc = vma::raii::VirtualAllocation { m_bufferManager.GetBuffer(m_indexBuffer).VirtualBlock(), allocationInfo };
        subMesh.indexCount = static_cast<std::uint32_t>(indices.size());

        vk::DeviceSize totalSize = vSize + iSize;
        if (stagingBuffer.Size() < totalSize) {
            stagingBuffer = CBuffer { m_context, totalSize,
                vk::BufferUsageFlagBits::eTransferSrc, MemoryLocation::eHostVisible
            };
        }

        std::memcpy(stagingBuffer.Data(), vertices.data(), vSize);
        std::memcpy(static_cast<std::uint8_t*>(stagingBuffer.Data()) + vSize, indices.data(), iSize);

        vk::raii::CommandBuffer cb = BeginSingleTimeCommands(*m_context.Device(), commandPool);
        {
            CCommandBuffer cmd { cb };
            cmd.Copy(m_bufferManager.GetBuffer(m_vertexBuffer), stagingBuffer, vSize, { 0, subMesh.VtxOffset() });
            cmd.Copy(m_bufferManager.GetBuffer(m_indexBuffer), stagingBuffer, iSize, { vSize, subMesh.IdxOffset() } );
        }
        EndSingleTimeCommands(m_context.Device(), cb);
    };

    UploadToPool("assets://matroskin.obj", m_matroskin);
    UploadToPool("assets://viking_room.obj", m_viking);
}
}
