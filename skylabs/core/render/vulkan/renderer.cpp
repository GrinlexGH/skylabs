#include <skylabs/core/render/vulkan/renderer.hpp>
#include <skylabs/public/logging.hpp>
#include <skylabs/core/render/vulkan/pipeline/shader.hpp>
#include <skylabs/core/render/vulkan/render_graph/graph.hpp>
#include <skylabs/core/camera.hpp>

#include <glm/gtx/hash.hpp>
#include <glm/ext/scalar_reciprocal.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <SDL3_image/SDL_image.h>
#include <tiny_obj_loader.h>

#include <chrono>
#include <ranges>
#include <random>

template<> struct std::hash<CVertex> {
    size_t operator()(const CVertex& vertex) const noexcept {
        return (hash<glm::vec3>()(vertex.m_position)) ^ (hash<glm::vec2>()(vertex.m_texCoord) << 1);
    }
};

struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
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

glm::mat4 ReverseZPerspective(int width, int height, float fov = 90) {
    glm::mat4 proj;
    float g = 1.0f / std::tan(0.5f * glm::radians(fov));
    proj = glm::mat4(0.0f);
    proj[0][0] = g / (static_cast<float>(width) / static_cast<float>(height));
    proj[1][1] = -g;
    proj[2][3] = -1.0f;
    proj[3][2] = 0.01f;

    return proj;
}

void UpdateUniformBuffer(
    const vk::Extent2D& cameraDimensions,
    Vulkan::CBuffer& uniformBuffer,
    const glm::mat4& view
) {
    // UBO
    UniformBufferObject ubo {
        .model = glm::mat4(1.0f),
        .view = view,
        .proj = ReverseZPerspective(cameraDimensions.width, cameraDimensions.height),
    };

    std::memcpy(uniformBuffer.Data(), &ubo, sizeof(ubo));
}
}

namespace Vulkan {
CRenderer::CRenderer(const IWindow* const window) {
    assert(window);

    m_context = CContext { window };
    m_surface = CSurface { m_context };
    m_swapchain = CSwapchain { m_context, *m_surface, 2, vk::PresentModeKHR::eMailbox };

    renderWidth = m_swapchain.Extent().width;
    renderHeight = m_swapchain.Extent().height;

    m_pipelineLayoutCache = CPipelineLayoutCache { m_context };

    m_graphicsCommands = CCommandBufferSet { m_context, m_context.Device().GraphicsQueue().FamilyIndex(), { FRAMES_IN_FLIGHT_COUNT, 0 } };
    m_computeCommands = CCommandBufferSet { m_context, m_context.Device().ComputeQueue().FamilyIndex(), { 0, 0 } };

    const std::uint32_t imageCount = m_swapchain.Images().size();
    m_renderFinishedSemaphores.reserve(imageCount);
    for (std::size_t i = 0; i < imageCount; ++i) {
        m_renderFinishedSemaphores.emplace_back(*m_context.Device(), vk::SemaphoreCreateInfo {});
    }

    m_frameData.reserve(FRAMES_IN_FLIGHT_COUNT);
    for (std::size_t i = 0; i < FRAMES_IN_FLIGHT_COUNT; ++i) {
        m_frameData.emplace_back(m_context);
    }

    m_graph = RG::CRenderGraph { m_context, FRAMES_IN_FLIGHT_COUNT };

    CBuffer stagingBuffer { nullptr };
    m_singleCommandPool = m_context.Device()->createCommandPool({
        vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        m_context.Device().GraphicsQueue().FamilyIndex()
    });

    m_mainSampler = CSampler { m_context };
    m_modelTextureSampler = CSampler {
        m_context, {
            .m_filtering = vk::Filter::eNearest,
            .m_mipmapFiltering = vk::SamplerMipmapMode::eNearest,
        }
    };

    m_textureManager = CTexturePool { m_context, m_swapchain.Extent(), FRAMES_IN_FLIGHT_COUNT };
    auto& txm = m_textureManager;

    m_colorBuffer = txm.CreateTexture("colorBuffer", {
        .m_extent = RelativeTextureSize {},
        .m_usage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
    });

    m_colorBufferMSAAx = txm.CreateTexture("colorBufferMSAAx", {
        .m_extent = RelativeTextureSize {},
        .m_usage = vk::ImageUsageFlagBits::eColorAttachment,
        .m_sampleCount = vk::SampleCountFlagBits::e8,
    });

    m_depthBufferMSAAx = txm.CreateTexture("depthBufferMSAAx", {
        .m_extent = RelativeTextureSize {},
        .m_format = vk::Format::eD32Sfloat,
        .m_usage = vk::ImageUsageFlagBits::eDepthStencilAttachment,
        .m_sampleCount = vk::SampleCountFlagBits::e8
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


    m_descriptorManager = CDescriptorPool{ m_context, FRAMES_IN_FLIGHT_COUNT };
    auto& dsm = m_descriptorManager;

    m_mainDescriptorSetMatroskin = dsm.CreateDescriptorSet({
        {
            .m_binding = 0,
            .m_type = vk::DescriptorType::eUniformBuffer,
            .m_shaderStages = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
            .m_info = BufferDescriptorInfo { .m_buffer = m_uniformBuffer }
        },
        {
            .m_binding = 1,
            .m_type = vk::DescriptorType::eCombinedImageSampler,
            .m_shaderStages = vk::ShaderStageFlagBits::eFragment,
            .m_info = SampledImageDescriptorInfo { .m_image = m_matroskinModelTexture, .m_sampler = *m_modelTextureSampler }
        }
    });

    m_mainDescriptorSetVikingRoom = dsm.CreateDescriptorSet({
        {
            .m_binding = 0,
            .m_type = vk::DescriptorType::eUniformBuffer,
            .m_shaderStages = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
            .m_info = BufferDescriptorInfo { .m_buffer = m_uniformBuffer }
        },
        {
            .m_binding = 1,
            .m_type = vk::DescriptorType::eCombinedImageSampler,
            .m_shaderStages = vk::ShaderStageFlagBits::eFragment,
            .m_info = SampledImageDescriptorInfo { .m_image = m_roomModelTexture, .m_sampler = *m_modelTextureSampler }
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

    // Main pipeline
    // Shaders
    const CShader vertexShader(m_context, vk::ShaderStageFlagBits::eVertex, "shader.vert.spv");
    const CShader fragmentShader(m_context, vk::ShaderStageFlagBits::eFragment, "shader.frag.spv");

    // Pipeline
    const vk::raii::PipelineLayout& mainPipelineLayout = m_pipelineLayoutCache.GetLayout({
        { *dsm.GetDescriptorSetLayout(m_mainDescriptorSetMatroskin), *dsm.GetDescriptorSetLayout(m_mainDescriptorSetVikingRoom) }
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
        .m_sampling =  vk::SampleCountFlagBits::e8
    }
    };

    // Swapchain pipeline
    // Shaders
    const CShader vertexShaderSwapchain(m_context, vk::ShaderStageFlagBits::eVertex, "shaderSwapchain.vert.spv");
    const CShader fragmentShaderSwapchain(m_context, vk::ShaderStageFlagBits::eFragment, "shaderSwapchain.frag.spv");

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

void CRenderer::Draw(glm::mat4 view, float) {
    m_textureManager.SetFrameIndex(m_frameIndex);
    m_bufferManager.SetFrameIndex(m_frameIndex);
    m_descriptorManager.SetFrameIndex(m_frameIndex);
    auto& cmd = m_graphicsCommands.PrimaryBuffers()[m_frameIndex];
    CFrame& frameData = m_frameData[m_frameIndex];
    const CDevice& device = m_context.Device();
    auto& uniformBuffer = m_bufferManager.GetBuffer(m_uniformBuffer);
    auto& vertexBuffer = m_bufferManager.GetBuffer(m_vertexBuffer);
    auto& indexBuffer = m_bufferManager.GetBuffer(m_indexBuffer);
    auto& colorBuffer = m_textureManager.GetTexture(m_colorBuffer);
    auto& colorBufferMSAA = m_textureManager.GetTexture(m_colorBufferMSAAx);
    auto& depthBufferMSAA = m_textureManager.GetTexture(m_depthBufferMSAAx);
    auto descriptorSetMainMatroskin = m_descriptorManager.GetDescriptorSet(m_mainDescriptorSetMatroskin);
    auto descriptorSetMainRoom = m_descriptorManager.GetDescriptorSet(m_mainDescriptorSetVikingRoom);
    auto descriptorSetSwapchain = m_descriptorManager.GetDescriptorSet(m_swapchainDescriptorSet);

    UpdateUniformBuffer(m_swapchain.Extent(), uniformBuffer, view);

    // Wait for fence to ensure that the previous frame rendering is finished
    vk::Result result = device->waitForFences({ frameData.GetFence() }, vk::True, std::numeric_limits<std::uint64_t>::max());
    if (result != vk::Result::eSuccess) {
        throw std::runtime_error("Failed to wait for fence: " + vk::to_string(result));
    }

    // Acquire next image from the swapchain
    auto acquireResult = m_swapchain.AcquireImage(*frameData.GetImageAvailableSemaphore());
    if (!acquireResult) {
        Resize(frameData);
        return;
    }

    std::uint32_t imageIndex = *acquireResult;

    // Reset fence after resizing to avoid deadlock on next invocation of Draw()
    device->resetFences({ frameData.GetFence() });

    static std::vector<bool> m_firstUse(FRAMES_IN_FLIGHT_COUNT, true);

    cmd->reset();
    cmd->begin({});

    if (m_firstUse[m_frameIndex]) {
        cmd.PipelineBarrier({
            ImageBarrier { colorBuffer, colorBuffer.FullRange(), Usage::eNone, Usage::eColorAttachment },
            ImageBarrier { colorBufferMSAA, colorBufferMSAA.FullRange(), Usage::eNone, Usage::eColorAttachment },
            ImageBarrier { depthBufferMSAA, depthBufferMSAA.FullRange(), Usage::eNone, Usage::eDepthWrite },
        });
    } else {
        cmd.PipelineBarrier({
            ImageBarrier { colorBuffer, colorBuffer.FullRange(), Usage::eColorAttachment, Usage::eColorAttachment },
            ImageBarrier { colorBufferMSAA, colorBufferMSAA.FullRange(), Usage::eColorAttachment, Usage::eColorAttachment },
            ImageBarrier { colorBuffer, colorBuffer.FullRange(), Usage::eSampledFragment, Usage::eColorAttachment },
        });
    }

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
    mainRenderInfo.renderArea = vk::Rect2D { { 0, 0 }, { renderWidth, renderHeight } };
    mainRenderInfo.layerCount = 1;
    mainRenderInfo.colorAttachmentCount = 1;
    mainRenderInfo.pColorAttachments = &colorAttachInfo;
    mainRenderInfo.pDepthAttachment = &depthAttachInfo;

    cmd->beginRendering(mainRenderInfo);
    cmd->bindPipeline(vk::PipelineBindPoint::eGraphics, *m_pipelineMain);
    auto viewport = vk::Viewport { 0.0f, 0.0f, static_cast<float>(renderWidth), static_cast<float>(renderHeight), 0.0f, 1.0f };
    cmd->setViewport(0, viewport);
    auto scissor = vk::Rect2D { { 0, 0 }, { renderWidth, renderHeight } };
    cmd->setScissor(0, scissor);
    cmd->setDepthBiasEnable(vk::False);
    cmd->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_pipelineMain.Layout(), 0, descriptorSetMainMatroskin, {});
    std::array<vk::Buffer, 1> vertexBuffers { *vertexBuffer };
    std::array<vk::DeviceSize, vertexBuffers.size()> offsets = { 0 };
    cmd->bindVertexBuffers(0, vertexBuffers, offsets);
    cmd->bindIndexBuffer(*indexBuffer, 0, vk::IndexType::eUint16);
    cmd->drawIndexed(m_matroskin.indexCount, 1, m_matroskin.firstIndex, m_matroskin.vertexOffset, 0);
    cmd->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_pipelineMain.Layout(), 0, descriptorSetMainRoom, {});
    cmd->drawIndexed(m_viking.indexCount, 1, m_viking.firstIndex, m_viking.vertexOffset, 0);
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
    viewport = vk::Viewport {0.0f, 0.0f, static_cast<float>(m_swapchain.Extent().width), static_cast<float>(m_swapchain.Extent().height), 0.0f, 1.0f};
    cmd->setViewport(0, viewport);
    scissor = vk::Rect2D {{0, 0}, m_swapchain.Extent()};
    cmd->setScissor(0, scissor);
    cmd->setDepthBiasEnable(vk::False);
    cmd->bindPipeline(vk::PipelineBindPoint::eGraphics, *m_pipelineSwapchain);
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
    result = m_swapchain.PresentImage(imageIndex, { *m_renderFinishedSemaphores[imageIndex] });
    if (result != vk::Result::eSuccess || IsResized()) {
        Resize(frameData);
        return;
    }

    m_frameIndex = (m_frameIndex + 1) % FRAMES_IN_FLIGHT_COUNT;
}

void CRenderer::Resize(CFrame& currentFrameData) {
    for (auto& frame : m_frameData) {
        vk::Result result = m_context.Device()->waitForFences({ frame.GetFence() }, vk::True, std::numeric_limits<std::uint64_t>::max());
        if (result != vk::Result::eSuccess) {
            throw std::runtime_error("Failed to wait for fence: " + vk::to_string(result));
        }
    }

    auto [width, height] = m_context.Window()->DrawableSize();
    renderWidth = width;
    renderHeight = height;

    m_swapchain = CSwapchain { std::move(m_swapchain), static_cast<std::uint32_t>(m_swapchain.Images().size()), m_swapchain.PresentMode() };

    m_textureManager.Resize(m_swapchain.Extent());
    m_descriptorManager.UpdateDescriptorSets(m_bufferManager, m_textureManager);

    currentFrameData.RecreateImageAvailableSemaphore();
    m_isResized = false;
}

void CRenderer::LoadModelTextures(CBuffer& stagingBuffer, const vk::raii::CommandPool& commandPool) {
    SDL_Surface* imageRaw = IMG_Load("assets/viking_room.png");
    if (!imageRaw) {
        throw std::runtime_error("Failed to load texture image");
    }
    SDL_Surface* image = SDL_ConvertSurface(imageRaw, SDL_PIXELFORMAT_ABGR8888);
    SDL_DestroySurface(imageRaw);
    vk::DeviceSize imageSize = static_cast<vk::DeviceSize>(image->w) * image->h * 4;

    if (stagingBuffer.Size() < imageSize) {
        stagingBuffer = CBuffer {
            m_context,
            imageSize,
            vk::BufferUsageFlagBits::eTransferSrc,
            MemoryLocation::eHostVisible
        };
    }
    std::memcpy(stagingBuffer.Data(), image->pixels, static_cast<std::size_t>(imageSize));

    CImage modelTexture { m_context, {
        .m_extent = vk::Extent3D { static_cast<uint32_t>(image->w), static_cast<uint32_t>(image->h), 1 },
        .m_format = vk::Format::eR8G8B8A8Srgb,
        .m_mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(image->w, image->h)))) + 1,
        .m_usageFlags = vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
    }};

    {
        vk::raii::CommandBuffer commandBuffer = BeginSingleTimeCommands(*m_context.Device(), commandPool);
            CCommandBuffer cmd { commandBuffer };
            cmd.PipelineBarrier({ ImageBarrier { modelTexture, modelTexture.FullRange(), Vulkan::Usage::eNone, Vulkan::Usage::eTransferWrite } });
            cmd.Copy(modelTexture, stagingBuffer);
            const vk::FormatProperties formatProperties = m_context.PhysicalDevice()->getFormatProperties(modelTexture.Format());
            if (!(formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear)) {
                throw std::runtime_error("texture image format does not support linear blitting");
            }
            cmd.GenerateMipmaps(modelTexture);
        EndSingleTimeCommands(m_context.Device(), commandBuffer);
    }

    m_roomModelTexture = m_textureManager.ImportTexture("VikingModelTexture", std::move(modelTexture));

    SDL_DestroySurface(image);

    imageRaw = IMG_Load("assets/matroskin.png");
    if (!imageRaw) {
        throw std::runtime_error("Failed to load texture image");
    }
    image = SDL_ConvertSurface(imageRaw, SDL_PIXELFORMAT_ABGR8888);
    SDL_DestroySurface(imageRaw);
    imageSize = static_cast<vk::DeviceSize>(image->w) * image->h * 4;

    if (stagingBuffer.Size() < imageSize) {
        stagingBuffer = CBuffer {
            m_context,
            imageSize,
            vk::BufferUsageFlagBits::eTransferSrc,
            MemoryLocation::eHostVisible
        };
    }
    std::memcpy(stagingBuffer.Data(), image->pixels, static_cast<std::size_t>(imageSize));

    modelTexture = CImage { m_context, {
        .m_extent = vk::Extent3D { static_cast<uint32_t>(image->w), static_cast<uint32_t>(image->h), 1 },
        .m_format = vk::Format::eR8G8B8A8Srgb,
        .m_mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(image->w, image->h)))) + 1,
        .m_usageFlags = vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
    }};

    {
        vk::raii::CommandBuffer commandBuffer = BeginSingleTimeCommands(*m_context.Device(), commandPool);
            CCommandBuffer cmd { commandBuffer };
            cmd.PipelineBarrier({ ImageBarrier { modelTexture, modelTexture.FullRange(), Vulkan::Usage::eNone, Vulkan::Usage::eTransferWrite } });
            cmd.Copy(modelTexture, stagingBuffer);
            const vk::FormatProperties formatProperties = m_context.PhysicalDevice()->getFormatProperties(modelTexture.Format());
            if (!(formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear)) {
                throw std::runtime_error("texture image format does not support linear blitting");
            }
            cmd.GenerateMipmaps(modelTexture);
        EndSingleTimeCommands(m_context.Device(), commandBuffer);
    }

    m_matroskinModelTexture = m_textureManager.ImportTexture("matroskinModelTexture", std::move(modelTexture));

    SDL_DestroySurface(image);
}

auto LoadModel(const char* filename) {
    std::vector<CVertex> vertices;
    std::vector<std::uint16_t> indices;

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn;
    std::string err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename)) {
        throw std::runtime_error(warn + " " + err);
    }

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
    auto [mVertices, mIndices] = LoadModel("assets/matroskin.obj");
    auto [vVertices, vIndices] = LoadModel("assets/viking_room.obj");

    m_matroskin.indexCount = static_cast<std::uint32_t>(mIndices.size());

    m_viking.indexCount = static_cast<std::uint32_t>(vIndices.size());
    m_viking.firstIndex = m_matroskin.indexCount;
    m_viking.vertexOffset = static_cast<std::int32_t>(mVertices.size());

    const vk::DeviceSize mVertexBufferSize = sizeof(mVertices[0]) * mVertices.size();
    const vk::DeviceSize vVertexBufferSize = sizeof(vVertices[0]) * vVertices.size();
    if (stagingBuffer.Size() < mVertexBufferSize + vVertexBufferSize) {
        stagingBuffer = CBuffer {
            m_context,
            mVertexBufferSize + vVertexBufferSize,
            vk::BufferUsageFlagBits::eTransferSrc,
            MemoryLocation::eHostVisible
        };
    }

    std::memcpy(stagingBuffer.Data(), mVertices.data(), static_cast<std::size_t>(mVertexBufferSize));
    std::memcpy(static_cast<char*>(stagingBuffer.Data()) + mVertexBufferSize, vVertices.data(), static_cast<std::size_t>(vVertexBufferSize));

    CBuffer vertexBuffer {
        m_context,
        mVertexBufferSize + vVertexBufferSize,
        vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
        MemoryLocation::eDeviceOnly
    };

    {
        vk::raii::CommandBuffer commandBuffer = BeginSingleTimeCommands(*m_context.Device(), commandPool);
        CCommandBuffer cmd { commandBuffer };
        cmd.Copy(vertexBuffer, stagingBuffer, mVertexBufferSize + vVertexBufferSize);
        EndSingleTimeCommands(m_context.Device(), commandBuffer);
    }

    const vk::DeviceSize mIndexBufferSize = sizeof(mIndices[0]) * mIndices.size();
    const vk::DeviceSize vIndexBufferSize = sizeof(vIndices[0]) * vIndices.size();
    if (stagingBuffer.Size() < mIndexBufferSize + vIndexBufferSize) {
        stagingBuffer = CBuffer {
            m_context,
            mIndexBufferSize + vIndexBufferSize,
            vk::BufferUsageFlagBits::eTransferSrc,
            MemoryLocation::eHostVisible
        };
    }

    std::memcpy(stagingBuffer.Data(), mIndices.data(), static_cast<std::size_t>(mIndexBufferSize));
    std::memcpy(static_cast<char*>(stagingBuffer.Data()) + mIndexBufferSize, vIndices.data(), static_cast<std::size_t>(vIndexBufferSize));

    CBuffer indexBuffer = {
        m_context,
        mIndexBufferSize + vIndexBufferSize,
        vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
        MemoryLocation::eDeviceOnly
    };

    {
        vk::raii::CommandBuffer commandBuffer = BeginSingleTimeCommands(*m_context.Device(), commandPool);
        CCommandBuffer cmd { commandBuffer };
        cmd.Copy(indexBuffer, stagingBuffer, mIndexBufferSize + vIndexBufferSize);
        EndSingleTimeCommands(m_context.Device(), commandBuffer);
    }

    m_vertexBuffer = m_bufferManager.ImportBuffer("vertexBuffer", std::move(vertexBuffer));
    m_indexBuffer = m_bufferManager.ImportBuffer("indexBuffer", std::move(indexBuffer));
}
}
