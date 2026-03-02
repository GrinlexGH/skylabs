#include <skylabs/core/render/vulkan/renderer.hpp>
#include <skylabs/public/logging.hpp>
#include <skylabs/core/render/vulkan/shader.hpp>
#include <skylabs/core/camera.hpp>

#include <glm/gtx/hash.hpp>
#include <glm/ext/scalar_reciprocal.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <SDL3_image/SDL_image.h>
#include <tiny_obj_loader.h>

#include <chrono>
#include <random>

template<> struct std::hash<CVertex> {
    size_t operator()(const CVertex& vertex) const noexcept {
        return (hash<glm::vec3>()(vertex.m_position)) ^ (hash<glm::vec2>()(vertex.m_texCoord) << 1);
    }
};

std::vector vertices {
    CVertex { .m_position = { -100.5f, -100.5f, 0.0f }, .m_texCoord = { 100.0f, 0.0f } },
    CVertex { .m_position = {  100.5f, -100.5f, 0.0f }, .m_texCoord = { 0.0f, 0.0f } },
    CVertex { .m_position = {  100.5f,  100.5f, 0.0f }, .m_texCoord = { 0.0f, 100.0f } },
    CVertex { .m_position = { -100.5f,  100.5f, 0.0f }, .m_texCoord = { 100.0f, 100.0f } },
};

struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
    glm::mat4 lightproj;
    glm::mat4 lightview;
};

struct LightUBO {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

glm::vec3 offset = {0.0f, 0.0f, 0.0f}; // то что реально уходит в UBO
glm::vec3 targetOffset  = {0.0f, 0.0f, 0.0f}; // то куда хотим прийти

constexpr float lerpSpeed = 5.0f; // чем больше, тем быстрее двигается

void MoveForward()  { targetOffset.z += 0.1f; }
void MoveBackward() { targetOffset.z -= 0.1f; }

std::vector<std::uint16_t> indices = {
    0, 1, 2, 2, 3, 0,
};

namespace {
std::pair<vk::Result, uint32_t> SwapchainNextImageWrapper(
    const vk::raii::SwapchainKHR& swapchain,
    const uint64_t timeout,
    const vk::Semaphore semaphore,
    const vk::Fence fence
) {
    uint32_t image_index;
    auto result = static_cast<vk::Result>(swapchain.getDispatcher()->vkAcquireNextImageKHR(
        static_cast<VkDevice>(swapchain.getDevice()), static_cast<VkSwapchainKHR>(*swapchain),
        timeout, static_cast<VkSemaphore>(semaphore), static_cast<VkFence>(fence), &image_index));
    return std::make_pair(result, image_index);
}

vk::Result QueuePresentWrapper(
    const vk::raii::Queue& queue,
    const vk::PresentInfoKHR& presentInfo
) {
    return static_cast<vk::Result>(queue.getDispatcher()->vkQueuePresentKHR(
        static_cast<VkQueue>(*queue), reinterpret_cast<const VkPresentInfoKHR*>(&presentInfo)));
}

vk::raii::CommandBuffer BeginSingleTimeCommands(
    const vk::raii::Device& device,
    const vk::CommandPool& commandPool
) {
    vk::CommandBufferAllocateInfo allocInfo {};
    allocInfo.level = vk::CommandBufferLevel::ePrimary;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    vk::raii::CommandBuffers commandBuffers { device, allocInfo };

    vk::CommandBufferBeginInfo beginInfo {};
    beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

    commandBuffers[0].begin(beginInfo);

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

    device.GraphicsQueue()->submit(submitInfo);
    device.GraphicsQueue()->waitIdle(); // TODO: USE FENCES

    commandBuffer.clear();
}

void GenerateMipmaps(
    const vk::raii::PhysicalDevice& physicalDevice,
    const vk::raii::CommandBuffer& cmd,
    const Vulkan::CImage& image
) {
    const vk::FormatProperties formatProperties = physicalDevice.getFormatProperties(image.Format());

    if (!(formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear)) {
        throw std::runtime_error("texture image format does not support linear blitting!");
    }

    vk::ImageMemoryBarrier barrier {};
    barrier.image = *image;
    barrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
    barrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;
    barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;

    int32_t mipWidth = static_cast<std::int32_t>(image.Extent().width);
    int32_t mipHeight = static_cast<std::int32_t>(image.Extent().height);

    for (uint32_t i = 1; i < image.MipLevels(); i++) {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
        barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;

        cmd.pipelineBarrier(
            vk::PipelineStageFlagBits::eTransfer,
            vk::PipelineStageFlagBits::eTransfer,
            {}, {}, {},
            barrier
        );

        vk::ImageBlit blit {};
        blit.srcOffsets[0] = {{ 0, 0, 0 }};
        blit.srcOffsets[1] = {{ mipWidth, mipHeight, 1 }};
        blit.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;
        blit.dstOffsets[0] = {{ 0, 0, 0 }};
        blit.dstOffsets[1] = {{ mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 }};
        blit.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1;

        cmd.blitImage(
            *image, vk::ImageLayout::eTransferSrcOptimal,
            *image, vk::ImageLayout::eTransferDstOptimal,
            blit,
            vk::Filter::eNearest
        );

        barrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
        barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

        cmd.pipelineBarrier(
            vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader,
            {}, {}, {},
            barrier
        );

        if (mipWidth > 1) mipWidth /= 2;
        if (mipHeight > 1) mipHeight /= 2;
    }

    barrier.subresourceRange.baseMipLevel = image.MipLevels() - 1;
    barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
    barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
    barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

    cmd.pipelineBarrier(
        vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, {},
        {}, {},
        barrier
    );
}

void CopyBuffer(
    const Vulkan::CDevice& device,
    const vk::raii::CommandPool& commandPool,
    const vk::Buffer srcBuffer,
    const vk::Buffer dstBuffer,
    const vk::DeviceSize size
) {
    vk::raii::CommandBuffer commandBuffer = BeginSingleTimeCommands(*device, commandPool);

    vk::BufferCopy copyRegion;
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size = size;

    commandBuffer.copyBuffer(srcBuffer, dstBuffer, copyRegion);

    EndSingleTimeCommands(device, commandBuffer);
}

void UpdateLightUniformBuffer(
    Vulkan::CBuffer& uniformBuffersMapped,
    const glm::mat4& lightView,
    const glm::mat4& lightProj,
    const float deltaTime
) {
    LightUBO ubo {};
    ubo.view = lightView;
    ubo.proj = lightProj;

    offset = glm::mix(offset, targetOffset, lerpSpeed * deltaTime);
    if (glm::length(targetOffset - offset) < 0.0001f) offset = targetOffset;
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, offset);
    model = glm::rotate(model, glm::radians(5.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.model = model;

    std::memcpy(uniformBuffersMapped.Data(), &ubo, sizeof(ubo));
}

void UpdateUniformBuffer(
    const vk::Extent2D& cameraDimensions,
    Vulkan::CBuffer& uniformBuffersMapped,
    const glm::mat4& view,
    const float deltaTime,
    const glm::mat4& lightProj,
    const glm::mat4& lightView
) {
    UniformBufferObject ubo {};
    ubo.model = glm::rotate(glm::mat4(1.0f), glm::radians(5.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.view = view;

    float g = 1.0f / std::tan(0.5f * glm::radians(90.0f));
    ubo.proj = glm::mat4(0.0f);
    ubo.proj[0][0] = g / (static_cast<float>(cameraDimensions.width) / static_cast<float>(cameraDimensions.height));
    ubo.proj[1][1] = -g;
    ubo.proj[2][2] = 0.0f;
    ubo.proj[2][3] = -1.0f;
    ubo.proj[3][2] = 0.01f;

    ubo.lightview = lightView;
    ubo.lightproj = lightProj;

    offset = glm::mix(offset, targetOffset, lerpSpeed * deltaTime);
    if (glm::length(targetOffset - offset) < 0.0001f) offset = targetOffset;

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, offset);
    model = glm::rotate(model, glm::radians(5.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.model = model;

    std::memcpy(uniformBuffersMapped.Data(), &ubo, sizeof(ubo));
}

std::uint32_t renderWidth = 0;
std::uint32_t renderHeight = 0;
}

namespace Vulkan {
CRenderer::CRenderer(const IWindow* const window) {
    assert(window);

    m_context = CContext { window };
    m_surface = CSurface { m_context };
    m_swapchain = CSwapchain { m_context, *m_surface, 2, vk::PresentModeKHR::eImmediate };

    renderWidth = m_swapchain.Extent().width;
    renderHeight = m_swapchain.Extent().height;

    const std::uint32_t imageCount = m_swapchain.ImageCount();
    m_renderFinishedSemaphores.reserve(imageCount);
    for (std::size_t i = 0; i < imageCount; ++i) {
        m_renderFinishedSemaphores.emplace_back(*m_context.Device(), vk::SemaphoreCreateInfo {});
    }

    m_frameData.reserve(FRAMES_IN_FLIGHT_COUNT);

    for (std::size_t i = 0; i < FRAMES_IN_FLIGHT_COUNT; ++i) {
        m_frameData.emplace_back(m_context);
    }

    CBuffer stagingBuffer { nullptr };
    m_singleCommandPool = m_context.Device()->createCommandPool({
        vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        m_context.Device().GraphicsQueue().FamilyIndex()
    });

    m_mainSampler = CSampler { m_context };
    m_computeSampler = CSampler { m_context };
    m_modelTextureSampler = CSampler { m_context };

    vk::SamplerCreateInfo createInfo {};
    createInfo.magFilter = vk::Filter::eLinear;
    createInfo.minFilter = vk::Filter::eLinear;
    createInfo.addressModeU = vk::SamplerAddressMode::eClampToBorder;
    createInfo.addressModeV = vk::SamplerAddressMode::eClampToBorder;
    createInfo.addressModeW = vk::SamplerAddressMode::eClampToBorder;
    createInfo.anisotropyEnable = vk::True;
    createInfo.maxAnisotropy = m_context.PhysicalDevice()->getProperties2().properties.limits.maxSamplerAnisotropy;
    createInfo.borderColor = vk::BorderColor::eFloatOpaqueBlack;
    createInfo.unnormalizedCoordinates = vk::False;
    createInfo.compareEnable = vk::True;
    createInfo.compareOp = vk::CompareOp::eGreaterOrEqual;
    createInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
    createInfo.mipLodBias = 0.0f;
    createInfo.minLod = 0.0f;
    createInfo.maxLod = vk::LodClampNone;
    m_samplerLight = vk::raii::Sampler { *m_context.Device(), createInfo };


    m_textureManager = RG::CTextureManager { m_context, { .m_width = renderWidth, .m_height = renderHeight }, FRAMES_IN_FLIGHT_COUNT };
    auto& txm = m_textureManager;

    m_colorBuffer = txm.CreateTexture("colorBuffer", {
        .m_usage = RG::TextureUsageBits::eAttachment | RG::TextureUsageBits::eSampled,
        .m_extent = RG::RelativeTextureSize {}
    });

    m_colorBufferMSAAx = txm.CreateTexture("colorBufferMSAAx", {
        .m_usage = RG::TextureUsageBits::eAttachment,
        .m_extent = RG::RelativeTextureSize {},
        .m_sampled = true
    });

    m_depthBufferMSAAx = txm.CreateTexture("depthBufferMSAAx", {
        .m_format = RG::TextureFormat::eDepthOptimal,
        .m_usage = RG::TextureUsageBits::eDepthAttachment,
        .m_extent = RG::RelativeTextureSize {},
        .m_sampled = true
    });

    m_computeBuffer = txm.CreateTexture("computePostProcess", {
        .m_format = RG::TextureFormat::eRGBA8888Unorm,
        .m_usage = RG::TextureUsageBits::eStorage | RG::TextureUsageBits::eSampled,
        .m_extent = RG::RelativeTextureSize {}
    });

    m_lightDepth = txm.CreateTexture("lightDepth", {
        .m_format = RG::TextureFormat::eDepthOptimal,
        .m_usage = RG::TextureUsageBits::eDepthAttachment | RG::TextureUsageBits::eSampled,
        .m_extent = RG::AbsoluteTextureSize { .m_width = 2048, .m_height = 2048 }
    });

    LoadModelTexture(stagingBuffer, m_singleCommandPool);

    txm.GenerateTextures();


    m_bufferManager = RG::CBufferManager { m_context, FRAMES_IN_FLIGHT_COUNT };
    auto& bfm = m_bufferManager;

    m_uniformBuffer = bfm.CreateBuffer("global-uniform", {
        .m_size = sizeof(UniformBufferObject),
        .m_location = MemoryLocation::eHostVisible,
        .m_usage = RG::BufferUsageFlagBits::eUniformBuffer,
        .m_isInFlight = true,
    });

    m_lightUBO = bfm.CreateBuffer("light-uniform", {
        .m_size = sizeof(LightUBO),
        .m_location = MemoryLocation::eHostVisible,
        .m_usage = RG::BufferUsageFlagBits::eUniformBuffer,
        .m_isInFlight = false,
    });

    LoadModel(stagingBuffer, m_singleCommandPool);

    bfm.GenerateBuffers();


    m_descriptorManager = RG::CDescriptorManager { m_context, FRAMES_IN_FLIGHT_COUNT };
    auto& dsm = m_descriptorManager;

    m_mainDescriptorSet = dsm.CreateDescriptorSet({{
        {
            .m_type = RG::DescriptorType::eUniformBuffer,
            .m_shaderStages = ShaderStageBits::eVertex | ShaderStageBits::eFragment,
            .m_info = RG::BufferDescriptorInfo { .m_buffer = m_uniformBuffer }
        },
        {
            .m_type = RG::DescriptorType::eCombinedImageSampler,
            .m_shaderStages = ShaderStageBits::eFragment,
            .m_info = RG::SampledImageDescriptorInfo { .m_image = m_modelTexture, .m_sampler = &*m_modelTextureSampler }
        },
        {
            .m_type = RG::DescriptorType::eCombinedImageSampler,
            .m_shaderStages = ShaderStageBits::eFragment,
            .m_info = RG::SampledImageDescriptorInfo { .m_image = m_lightDepth, .m_sampler = &m_samplerLight }
        },
    }});

    m_computeDescriptorSet = dsm.CreateDescriptorSet({{
        {
            .m_type = RG::DescriptorType::eStorageImage,
            .m_shaderStages = ShaderStageBits::eCompute,
            .m_info = RG::StorageImageDescriptorInfo { .m_image = m_computeBuffer }
        },
    }});

    m_swapchainDescriptorSet = dsm.CreateDescriptorSet({{
        {
            .m_type = RG::DescriptorType::eCombinedImageSampler,
            .m_shaderStages = ShaderStageBits::eFragment,
            .m_info = RG::SampledImageDescriptorInfo { .m_image = m_colorBuffer, .m_sampler = &*m_mainSampler }
        },
        {
            .m_type = RG::DescriptorType::eCombinedImageSampler,
            .m_shaderStages = ShaderStageBits::eFragment,
            .m_info = RG::SampledImageDescriptorInfo { .m_image = m_computeBuffer, .m_sampler = &*m_computeSampler }
        },
    }});

    m_lightDescriptorSet = dsm.CreateDescriptorSet({{
        {
            .m_type = RG::DescriptorType::eUniformBuffer,
            .m_shaderStages = ShaderStageBits::eVertex | ShaderStageBits::eFragment,
            .m_info = RG::BufferDescriptorInfo { .m_buffer = m_lightUBO }
        },
    }});

    dsm.CreateDescriptorPool();
    dsm.CreateDescriptorSets();
    dsm.UpdateDescriptorSets(m_bufferManager, m_textureManager);

    // Light pipeline
    const CShader vertexShaderLight(m_context, ShaderStageBits::eVertex, "light.vert.spv");

    const std::array<vk::PipelineShaderStageCreateInfo, 1> shaderStagesLight = {
        vertexShaderLight.PipelineShaderCreateInfo(),
    };

    // Pipeline
    vk::PipelineRenderingCreateInfo renderingInfoLight {};
    renderingInfoLight.depthAttachmentFormat = txm.GetTexture(m_lightDepth).Format();

    m_lightPipeline = CPipeline {
        m_context,
        shaderStagesLight,
        std::array { *dsm.GetDescriptorSetLayout(m_lightDescriptorSet) },
        CVertexFormat { CVertex::GetAttributes() },
        renderingInfoLight,
        vk::SampleCountFlagBits::e1
    };


    // Main pipeline
    // Shaders
    const CShader vertexShader(m_context, ShaderStageBits::eVertex, "shader.vert.spv");
    const CShader fragmentShader(m_context, ShaderStageBits::eFragment, "shader.frag.spv");

    const std::array<vk::PipelineShaderStageCreateInfo, 2> shaderStages = {
        vertexShader.PipelineShaderCreateInfo(),
        fragmentShader.PipelineShaderCreateInfo(),
    };

    // Pipeline
    std::array<vk::Format, 1> colorFormats = { txm.GetTexture(m_colorBuffer).Format() };
    vk::PipelineRenderingCreateInfo renderingInfo {};
    renderingInfo.colorAttachmentCount = static_cast<std::uint32_t>(colorFormats.size());
    renderingInfo.pColorAttachmentFormats = colorFormats.data();
    renderingInfo.depthAttachmentFormat = txm.GetTexture(m_depthBufferMSAAx).Format();

    m_pipelineMain = CPipeline {
        m_context,
        shaderStages,
        std::array { *dsm.GetDescriptorSetLayout(m_mainDescriptorSet) },
        CVertexFormat { CVertex::GetAttributes() },
        renderingInfo,
        vk::SampleCountFlagBits::e8
    };


    // Compute pipeline
    // Shaders
    CShader computeShader(m_context, ShaderStageBits::eCompute, "shader.comp.spv");

    // Pipeline
    vk::PipelineLayoutCreateInfo pipelineLayoutInfo {};
    std::array computeDescriptorSetLayouts = { *dsm.GetDescriptorSetLayout(m_computeDescriptorSet) };
    pipelineLayoutInfo.setLayoutCount = static_cast<std::uint32_t>(computeDescriptorSetLayouts.size());
    pipelineLayoutInfo.pSetLayouts = computeDescriptorSetLayouts.data();
    m_computePipelineLayout = m_context.Device()->createPipelineLayout(pipelineLayoutInfo);

    vk::ComputePipelineCreateInfo computePipelineCreateInfo {};
    computePipelineCreateInfo.stage = computeShader.PipelineShaderCreateInfo();
    computePipelineCreateInfo.layout = m_computePipelineLayout;
    m_computePipeline = m_context.Device()->createComputePipeline(nullptr, computePipelineCreateInfo);


    // Swapchain pipeline
    // Shaders
    const CShader vertexShaderSwapchain(m_context, ShaderStageBits::eVertex, "shaderSwapchain.vert.spv");
    const CShader fragmentShaderSwapchain(m_context, ShaderStageBits::eFragment, "shaderSwapchain.frag.spv");

    const std::array shaderStagesSwapchain = {
        vertexShaderSwapchain.PipelineShaderCreateInfo(),
        fragmentShaderSwapchain.PipelineShaderCreateInfo(),
    };

    // Pipeline
    std::array<vk::Format, 1> swapchainColorFormats { m_swapchain.SurfaceFormat().format };
    renderingInfo = vk::PipelineRenderingCreateInfo {};
    renderingInfo.colorAttachmentCount = static_cast<std::uint32_t>(swapchainColorFormats.size());
    renderingInfo.pColorAttachmentFormats = swapchainColorFormats.data();

    m_pipelineSwapchain = CPipeline {
        m_context,
        shaderStagesSwapchain,
        std::array { *dsm.GetDescriptorSetLayout(m_swapchainDescriptorSet) },
        CVertexFormat {{}},
        renderingInfo,
        vk::SampleCountFlagBits::e1
    };

    // Release barriers to finish a cirlcle in the Draw method
    ReleaseComputeBuffers();
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

void CRenderer::Draw(glm::mat4 view, float deltaTime) {
    m_textureManager.SetFrameIndex(m_frameIndex);
    m_bufferManager.SetFrameIndex(m_frameIndex);
    m_descriptorManager.SetFrameIndex(m_frameIndex);
    CFrameData& frameData = m_frameData[m_frameIndex];
    const CDevice& device = m_context.Device();
    auto& cmdMain = frameData.GetGraphicsCommandBuffers()[0];
    auto& cmdComp = frameData.GetComputeCommandBuffers()[0];
    auto& cmdFina = frameData.GetGraphicsCommandBuffers()[1];
    auto& cmdLight = frameData.GetGraphicsCommandBuffers()[2];
    auto& semMain = frameData.GetSemaphores()[0];
    auto& semComp = frameData.GetSemaphores()[1];
    auto& semLight = frameData.GetSemaphores()[2];
    auto& colorBuffer = m_textureManager.GetTexture(m_colorBuffer);
    auto& colorBufferMSAA = m_textureManager.GetTexture(m_colorBufferMSAAx);
    auto& depthBufferMSAA = m_textureManager.GetTexture(m_depthBufferMSAAx);
    auto& computeBuffer = m_textureManager.GetTexture(m_computeBuffer);
    auto& uniformBuffer = m_bufferManager.GetBuffer(m_uniformBuffer);
    auto& vertexBuffer = m_bufferManager.GetBuffer(m_vertexBuffer);
    auto& indexBuffer = m_bufferManager.GetBuffer(m_indexBuffer);
    auto& lightBuffer = m_textureManager.GetTexture(m_lightDepth);
    auto& lightUBO = m_bufferManager.GetBuffer(m_lightUBO);
    auto descriptorSetMain = m_descriptorManager.GetDescriptorSet(m_mainDescriptorSet);
    auto descriptorSetCompute = m_descriptorManager.GetDescriptorSet(m_computeDescriptorSet);
    auto descriptorSetSwapchain = m_descriptorManager.GetDescriptorSet(m_swapchainDescriptorSet);
    auto descriptorSetLight = m_descriptorManager.GetDescriptorSet(m_lightDescriptorSet);

    static float yO = -600;
    // 1. Настраиваем камеру света
    static CCamera lightCamera{ {1, 0, 1} };
    lightCamera.ProcessMouseMovement(0, yO);
    yO = 0;
    float g = 1.0f / std::tan(0.5f * glm::radians(90.0f));
    auto lightProj = glm::mat4(0.0f);
    lightProj[0][0] = g / 1;
    lightProj[1][1] = -g;
    lightProj[2][2] = 0.0f;
    lightProj[2][3] = -1.0f;
    lightProj[3][2] = 0.01f;

    glm::mat4 lightView = lightCamera.GetViewMatrix();

    UpdateLightUniformBuffer(lightUBO, lightView, lightProj, deltaTime);
    UpdateUniformBuffer(m_swapchain.Extent(), uniformBuffer, view, deltaTime, lightProj, lightView);

    // Wait for fence to ensure that the previous frame rendering is finished
    vk::Result result = device->waitForFences({ frameData.GetFence() }, vk::True, std::numeric_limits<std::uint64_t>::max());
    if (result != vk::Result::eSuccess) {
        throw std::runtime_error("Failed to wait for fence: " + vk::to_string(result));
    }

    // Acquire next image from the swapchain
    std::uint32_t imageIndex;
    std::tie(result, imageIndex) = SwapchainNextImageWrapper(
        *m_swapchain,
        std::numeric_limits<std::uint64_t>::max(),
        *frameData.GetImageAvailableSemaphore(), nullptr
    );

    if (result == vk::Result::eErrorOutOfDateKHR) {
        Resize(frameData);
        return;
    }

    if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR) {
        throw std::runtime_error("Failed to acquire swapchain image: " + vk::to_string(result));
    }

    // Reset fence after resizing to avoid deadlock on next invocation of Draw()
    device->resetFences({ frameData.GetFence() });

    // Recording render commands
    cmdLight.reset();
    cmdLight.begin({});

    // Prepare attachments for main render
    lightBuffer.TransitionLayout(cmdLight, vk::ImageLayout::eDepthStencilAttachmentOptimal);

    // light render
    vk::RenderingAttachmentInfo depthAttachInfoLight {};
    depthAttachInfoLight.imageView = lightBuffer.View();
    depthAttachInfoLight.imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
    depthAttachInfoLight.loadOp = vk::AttachmentLoadOp::eClear;
    depthAttachInfoLight.storeOp = vk::AttachmentStoreOp::eStore;
    depthAttachInfoLight.clearValue.depthStencil = vk::ClearDepthStencilValue{0.0f, 0};

    vk::RenderingInfo lightRenderInfo {};
    lightRenderInfo.renderArea = vk::Rect2D { { 0, 0 }, { 2048, 2048 } };
    lightRenderInfo.layerCount = 1;
    lightRenderInfo.pDepthAttachment = &depthAttachInfoLight;

    cmdLight.beginRendering(lightRenderInfo);
    cmdLight.bindPipeline(vk::PipelineBindPoint::eGraphics, *m_lightPipeline);
    vk::Viewport viewport { 0.0f, 0.0f, static_cast<float>(2048), static_cast<float>(2048), 0.0f, 1.0f };
    cmdLight.setViewport(0, viewport);
    vk::Rect2D scissor { { 0, 0 }, { 2048, 2048 } };
    cmdLight.setScissor(0, scissor);
    std::array<vk::Buffer, 1> vertexBuffers { *vertexBuffer };
    cmdLight.setDepthBiasEnable(vk::True);
    cmdLight.setDepthBias(-1.25f, 0.0f, -1.75f);
    std::array<vk::DeviceSize, vertexBuffers.size()> offsets = { 0 };
    cmdLight.bindVertexBuffers(0, vertexBuffers, offsets);
    cmdLight.bindIndexBuffer(*indexBuffer, 0, vk::IndexType::eUint16);
    cmdLight.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *m_lightPipeline.Layout(), 0, descriptorSetLight, {});
    cmdLight.drawIndexed(static_cast<std::uint32_t>(indices.size()), 1, 0, 0, 0);
    cmdLight.endRendering();
    cmdLight.end();

    vk::SubmitInfo lightSubmit {};
    lightSubmit.waitSemaphoreCount = 0;
    lightSubmit.pWaitSemaphores = nullptr;
    lightSubmit.pWaitDstStageMask = nullptr;
    lightSubmit.commandBufferCount = 1;
    lightSubmit.pCommandBuffers = &*cmdLight;
    lightSubmit.signalSemaphoreCount = 1;
    lightSubmit.pSignalSemaphores = &*semLight;
    device.GraphicsQueue()->submit(lightSubmit);

    // Recording render commands
    cmdMain.reset();
    cmdMain.begin({});

    // Prepare attachments for main render
    colorBuffer.TransitionLayout(cmdMain, vk::ImageLayout::eColorAttachmentOptimal);
    colorBufferMSAA.TransitionLayout(cmdMain, vk::ImageLayout::eColorAttachmentOptimal);
    depthBufferMSAA.TransitionLayout(cmdMain, vk::ImageLayout::eDepthStencilAttachmentOptimal);
    lightBuffer.TransitionLayout(cmdMain, vk::ImageLayout::eShaderReadOnlyOptimal);

    // Main render
    vk::RenderingAttachmentInfo colorAttachInfo {};
    colorAttachInfo.imageView = colorBufferMSAA.View();
    colorAttachInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
    colorAttachInfo.loadOp = vk::AttachmentLoadOp::eClear;
    colorAttachInfo.storeOp = vk::AttachmentStoreOp::eStore;
    colorAttachInfo.clearValue.color =
        vk::ClearColorValue(std::array<float, 4>{
            11.0f / 255.0f,
            16.0f / 255.0f,
            38.0f / 255.0f,
            1.0f
        });
    colorAttachInfo.resolveImageView = colorBuffer.View();
    colorAttachInfo.resolveImageLayout = vk::ImageLayout::eColorAttachmentOptimal;
    colorAttachInfo.resolveMode = vk::ResolveModeFlagBits::eAverage;

    vk::RenderingAttachmentInfo depthAttachInfo {};
    depthAttachInfo.imageView = depthBufferMSAA.View();
    depthAttachInfo.imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
    depthAttachInfo.loadOp = vk::AttachmentLoadOp::eClear;
    depthAttachInfo.storeOp = vk::AttachmentStoreOp::eDontCare;
    depthAttachInfo.clearValue.depthStencil = vk::ClearDepthStencilValue{0.0f, 0};

    vk::RenderingInfo mainRenderInfo {};
    mainRenderInfo.renderArea = vk::Rect2D { { 0, 0 }, { renderWidth, renderHeight } };
    mainRenderInfo.layerCount = 1;
    mainRenderInfo.colorAttachmentCount = 1;
    mainRenderInfo.pColorAttachments = &colorAttachInfo;
    mainRenderInfo.pDepthAttachment = &depthAttachInfo;

    cmdMain.beginRendering(mainRenderInfo);
    cmdMain.bindPipeline(vk::PipelineBindPoint::eGraphics, *m_pipelineMain);
    viewport = vk::Viewport { 0.0f, 0.0f, static_cast<float>(renderWidth), static_cast<float>(renderHeight), 0.0f, 1.0f };
    cmdMain.setViewport(0, viewport);
    scissor = vk::Rect2D { { 0, 0 }, { renderWidth, renderHeight } };
    cmdMain.setScissor(0, scissor);
    cmdMain.setDepthBiasEnable(vk::False);
    cmdMain.bindVertexBuffers(0, vertexBuffers, offsets);
    cmdMain.bindIndexBuffer(*indexBuffer, 0, vk::IndexType::eUint16);
    cmdMain.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *m_pipelineMain.Layout(), 0, descriptorSetMain, {});
    cmdMain.drawIndexed(static_cast<std::uint32_t>(indices.size()), 1, 0, 0, 0);
    cmdMain.endRendering();
    cmdMain.end();


    std::array<vk::PipelineStageFlags, 1> waitStagesMain = {
        vk::PipelineStageFlagBits::eFragmentShader
    };
    vk::SubmitInfo mainSubmit {};
    mainSubmit.waitSemaphoreCount = 1;
    mainSubmit.pWaitSemaphores = &*semLight;
    mainSubmit.pWaitDstStageMask = waitStagesMain.data();
    mainSubmit.commandBufferCount = 1;
    mainSubmit.pCommandBuffers = &*cmdMain;
    mainSubmit.signalSemaphoreCount = 1;
    mainSubmit.pSignalSemaphores = &*semMain;
    device.GraphicsQueue()->submit(mainSubmit);

    // Compute
    cmdComp.reset();
    cmdComp.begin({});

    vk::ImageMemoryBarrier2 acquireCompute {};
    acquireCompute.srcStageMask = vk::PipelineStageFlagBits2::eNone;
    acquireCompute.srcAccessMask = vk::AccessFlagBits2::eNone;
    acquireCompute.dstStageMask = vk::PipelineStageFlagBits2::eComputeShader;
    acquireCompute.dstAccessMask = vk::AccessFlagBits2::eShaderWrite;
    acquireCompute.oldLayout = computeBuffer.Layout();
    acquireCompute.newLayout = vk::ImageLayout::eGeneral;
    acquireCompute.srcQueueFamilyIndex = device.GraphicsQueue().FamilyIndex();
    acquireCompute.dstQueueFamilyIndex = device.ComputeQueue().FamilyIndex();
    acquireCompute.image = *computeBuffer;
    acquireCompute.subresourceRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 };

    vk::DependencyInfo depAcquireCompute {};
    depAcquireCompute.imageMemoryBarrierCount = 1;
    depAcquireCompute.pImageMemoryBarriers = &acquireCompute;
    cmdComp.pipelineBarrier2(depAcquireCompute);

    cmdComp.bindPipeline(vk::PipelineBindPoint::eCompute, m_computePipeline);
    cmdComp.bindDescriptorSets(vk::PipelineBindPoint::eCompute, m_computePipelineLayout, 0, descriptorSetCompute, {});
    cmdComp.dispatch((renderWidth + 7) / 8, (renderHeight + 7) / 8, 1);

    vk::ImageMemoryBarrier2 releaseBarrier {};
    releaseBarrier.srcStageMask = vk::PipelineStageFlagBits2::eComputeShader;
    releaseBarrier.srcAccessMask = vk::AccessFlagBits2::eShaderWrite;
    releaseBarrier.dstStageMask = vk::PipelineStageFlagBits2::eNone;
    releaseBarrier.dstAccessMask = vk::AccessFlagBits2::eNone;
    releaseBarrier.oldLayout = vk::ImageLayout::eGeneral;
    releaseBarrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    releaseBarrier.srcQueueFamilyIndex = device.ComputeQueue().FamilyIndex();
    releaseBarrier.dstQueueFamilyIndex = device.GraphicsQueue().FamilyIndex();
    releaseBarrier.image = *computeBuffer;
    releaseBarrier.subresourceRange = vk::ImageSubresourceRange{ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 };

    vk::DependencyInfo releaseDependencyInfo {};
    releaseDependencyInfo.imageMemoryBarrierCount = 1;
    releaseDependencyInfo.pImageMemoryBarriers = &releaseBarrier;

    cmdComp.pipelineBarrier2(releaseDependencyInfo);

    cmdComp.end();

    vk::SubmitInfo compSubmit {};
    compSubmit.waitSemaphoreCount = 0;
    compSubmit.commandBufferCount = 1;
    compSubmit.pCommandBuffers = &*cmdComp;
    compSubmit.signalSemaphoreCount = 1;
    compSubmit.pSignalSemaphores = &*semComp;
    device.ComputeQueue()->submit(compSubmit);

    // Prepare attachments for swapchain read
    cmdFina.reset();
    cmdFina.begin({});

    vk::ImageMemoryBarrier2 acquireGraphics {};
    acquireGraphics.srcStageMask = vk::PipelineStageFlagBits2::eNone;
    acquireGraphics.srcAccessMask = vk::AccessFlagBits2::eNone;
    acquireGraphics.dstStageMask = vk::PipelineStageFlagBits2::eFragmentShader;
    acquireGraphics.dstAccessMask = vk::AccessFlagBits2::eShaderRead;
    acquireGraphics.oldLayout = vk::ImageLayout::eGeneral;
    acquireGraphics.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    acquireGraphics.srcQueueFamilyIndex = device.ComputeQueue().FamilyIndex();
    acquireGraphics.dstQueueFamilyIndex = device.GraphicsQueue().FamilyIndex();
    acquireGraphics.image = *computeBuffer;
    acquireGraphics.subresourceRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 };

    vk::DependencyInfo depAcquireGraph {};
    depAcquireGraph.imageMemoryBarrierCount = 1;
    depAcquireGraph.pImageMemoryBarriers = &acquireGraphics;
    cmdFina.pipelineBarrier2(depAcquireGraph);

    colorBuffer.TransitionLayout(cmdFina, vk::ImageLayout::eShaderReadOnlyOptimal);
    CImage::CmdTransitionLayout(
        cmdFina,
        m_swapchain.Images()[imageIndex],
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eColorAttachmentOptimal
    );

    // Render to fullscreen triangle
    vk::RenderingAttachmentInfo swapchainAttachInfo {};
    swapchainAttachInfo.imageView = m_swapchain.ImageViews()[imageIndex];
    swapchainAttachInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
    swapchainAttachInfo.loadOp = vk::AttachmentLoadOp::eClear;
    swapchainAttachInfo.storeOp = vk::AttachmentStoreOp::eStore;
    swapchainAttachInfo.clearValue.color = std::array { 0.0f, 0.0f, 0.0f, 1.0f };

    vk::RenderingInfo swapchainRenderInfo {};
    swapchainRenderInfo.renderArea = vk::Rect2D{ {0, 0}, m_swapchain.Extent() };
    swapchainRenderInfo.layerCount = 1;
    swapchainRenderInfo.colorAttachmentCount = 1;
    swapchainRenderInfo.pColorAttachments = &swapchainAttachInfo;

    cmdFina.beginRendering(swapchainRenderInfo);
    viewport = vk::Viewport {0.0f, 0.0f, static_cast<float>(m_swapchain.Extent().width), static_cast<float>(m_swapchain.Extent().height), 0.0f, 1.0f};
    cmdFina.setViewport(0, viewport);
    scissor = vk::Rect2D {{0, 0}, m_swapchain.Extent()};
    cmdFina.setScissor(0, scissor);
    cmdFina.setDepthBiasEnable(vk::False);
    cmdFina.bindPipeline(vk::PipelineBindPoint::eGraphics, *m_pipelineSwapchain);
    cmdFina.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *m_pipelineSwapchain.Layout(), 0, descriptorSetSwapchain, {});
    cmdFina.draw(3, 1, 0, 0);
    cmdFina.endRendering();

    vk::ImageMemoryBarrier2 releaseGraphics {};
    releaseGraphics.srcStageMask = vk::PipelineStageFlagBits2::eFragmentShader;
    releaseGraphics.srcAccessMask = vk::AccessFlagBits2::eShaderRead;
    releaseGraphics.dstStageMask = vk::PipelineStageFlagBits2::eNone;
    releaseGraphics.dstAccessMask = vk::AccessFlagBits2::eNone;
    releaseGraphics.oldLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    releaseGraphics.newLayout = vk::ImageLayout::eGeneral;
    releaseGraphics.srcQueueFamilyIndex = device.GraphicsQueue().FamilyIndex();
    releaseGraphics.dstQueueFamilyIndex = device.ComputeQueue().FamilyIndex();
    releaseGraphics.image = *computeBuffer;
    releaseGraphics.subresourceRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 };

    vk::DependencyInfo depReleaseGraph {};
    depReleaseGraph.imageMemoryBarrierCount = 1;
    depReleaseGraph.pImageMemoryBarriers = &releaseGraphics;
    cmdFina.pipelineBarrier2(depReleaseGraph);

    computeBuffer.SetLayout(vk::ImageLayout::eShaderReadOnlyOptimal);

    // Prepare for presentation
    CImage::CmdTransitionLayout(
        cmdFina,
        m_swapchain.Images()[imageIndex],
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::ImageLayout::ePresentSrcKHR
    );

    cmdFina.end();

    // Submit command buffer
    std::array<vk::Semaphore, 3> waitSems = { *frameData.GetImageAvailableSemaphore(), semMain, semComp };
    std::array<vk::PipelineStageFlags, waitSems.size()> waitStages = {
        vk::PipelineStageFlagBits::eColorAttachmentOutput,
        vk::PipelineStageFlagBits::eColorAttachmentOutput,
        vk::PipelineStageFlagBits::eFragmentShader
    };
    vk::SubmitInfo finalSubmit{};
    finalSubmit.waitSemaphoreCount = static_cast<std::uint32_t>(waitSems.size());
    finalSubmit.pWaitSemaphores = waitSems.data();
    finalSubmit.pWaitDstStageMask = waitStages.data();
    finalSubmit.commandBufferCount = 1;
    finalSubmit.pCommandBuffers = &*cmdFina;
    finalSubmit.signalSemaphoreCount = 1;
    finalSubmit.pSignalSemaphores = &*m_renderFinishedSemaphores[imageIndex];
    device.GraphicsQueue()->submit(finalSubmit, frameData.GetFence());

    // Present
    vk::PresentInfoKHR presentInfo {};
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &*m_renderFinishedSemaphores[imageIndex];
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &**m_swapchain;
    presentInfo.pImageIndices = &imageIndex;
    result = QueuePresentWrapper(*device.PresentQueue(), presentInfo);
    if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR || IsResized()) {
        Resize(frameData);
        return;
    }

    m_frameIndex = (m_frameIndex + 1) % FRAMES_IN_FLIGHT_COUNT;
}

void CRenderer::Resize(CFrameData& currentFrameData) {
    for (auto& frame : m_frameData) {
        vk::Result result = m_context.Device()->waitForFences({ frame.GetFence() }, vk::True, std::numeric_limits<std::uint64_t>::max());
        if (result != vk::Result::eSuccess) {
            throw std::runtime_error("Failed to wait for fence: " + vk::to_string(result));
        }
    }

    auto [width, height] = m_context.Window()->DrawableSize();
    renderWidth = width;
    renderHeight = height;

    m_textureManager.Resize({ .m_width = renderWidth, .m_height = renderHeight });

    ReleaseComputeBuffers();

    m_descriptorManager.UpdateDescriptorSets(m_bufferManager, m_textureManager);

    currentFrameData.RecreateImageAvailableSemaphore();
    m_swapchain.Recreate();
    m_isResized = false;
}

void CRenderer::ReleaseComputeBuffers() {
    auto cmd = BeginSingleTimeCommands(*m_context.Device(), m_singleCommandPool);
    for (std::size_t i = 0; i < FRAMES_IN_FLIGHT_COUNT; ++i) {
        vk::ImageMemoryBarrier2 releaseBarrier {};
        releaseBarrier.srcStageMask = vk::PipelineStageFlagBits2::eNone;
        releaseBarrier.srcAccessMask = vk::AccessFlagBits2::eNone;
        releaseBarrier.dstStageMask = vk::PipelineStageFlagBits2::eNone;
        releaseBarrier.dstAccessMask = vk::AccessFlagBits2::eNone;
        releaseBarrier.oldLayout = vk::ImageLayout::eUndefined;
        releaseBarrier.newLayout = vk::ImageLayout::eGeneral;
        releaseBarrier.srcQueueFamilyIndex = m_context.Device().GraphicsQueue().FamilyIndex();
        releaseBarrier.dstQueueFamilyIndex = m_context.Device().ComputeQueue().FamilyIndex();
        releaseBarrier.image = *m_textureManager.GetTexture(m_computeBuffer, static_cast<int>(i));
        releaseBarrier.subresourceRange = vk::ImageSubresourceRange{ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 };

        vk::DependencyInfo releaseDependencyInfo {};
        releaseDependencyInfo.imageMemoryBarrierCount = 1;
        releaseDependencyInfo.pImageMemoryBarriers = &releaseBarrier;

        cmd.pipelineBarrier2(releaseDependencyInfo);
    }
    EndSingleTimeCommands(m_context.Device(), cmd);
}

void CRenderer::LoadModelTexture(CBuffer& stagingBuffer, const vk::raii::CommandPool& commandPool) {
    SDL_Surface* imageRaw = IMG_Load("assets/viking_room.png");
    if (!imageRaw) {
        throw std::runtime_error("Failed to load texture image!");
    }
    SDL_Surface* image = SDL_ConvertSurface(imageRaw, SDL_PIXELFORMAT_ABGR8888);
    SDL_DestroySurface(imageRaw);
    const vk::DeviceSize imageSize = static_cast<vk::DeviceSize>(image->w) * image->h * 4;

    if (stagingBuffer.Size() < imageSize) {
        stagingBuffer = CBuffer {
            m_context,
            imageSize,
            vk::BufferUsageFlagBits::eTransferSrc,
            MemoryLocation::eHostVisible
        };
    }
    std::memcpy(stagingBuffer.Data(), image->pixels, imageSize);

    CImage modelTexture = {
        m_context,
        vk::Extent2D { static_cast<uint32_t>(image->w), static_cast<uint32_t>(image->h) },
        vk::Format::eR8G8B8A8Srgb,
        vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
        vk::ImageAspectFlagBits::eColor,
        static_cast<uint32_t>(std::floor(std::log2(std::max(image->w, image->h)))) + 1
    };

    vk::raii::CommandBuffer commandBuffer = BeginSingleTimeCommands(*m_context.Device(), commandPool);
    {
        modelTexture.TransitionLayout(commandBuffer, vk::ImageLayout::eTransferDstOptimal);
        modelTexture.CopyBufferToImage(commandBuffer, *stagingBuffer, vk::Extent2D { static_cast<uint32_t>(image->w), static_cast<uint32_t>(image->h) });
        GenerateMipmaps(*m_context.PhysicalDevice(), commandBuffer, modelTexture);
    }
    EndSingleTimeCommands(m_context.Device(), commandBuffer);

    m_modelTexture = m_textureManager.ImportTexture("ModelTexture", std::move(modelTexture));

    SDL_DestroySurface(image);
}

void CRenderer::LoadModel(CBuffer& stagingBuffer, const vk::raii::CommandPool& commandPool) {
    const std::string MODEL_PATH = "assets/viking_room.obj";
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn;
    std::string err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, MODEL_PATH.c_str())) {
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

    const vk::DeviceSize vertexBufferSize = sizeof(vertices[0]) * vertices.size();
    if (stagingBuffer.Size() < vertexBufferSize) {
        stagingBuffer = CBuffer {
            m_context,
            vertexBufferSize,
            vk::BufferUsageFlagBits::eTransferSrc,
            MemoryLocation::eHostVisible
        };
    }

    std::memcpy(stagingBuffer.Data(), vertices.data(), vertexBufferSize);

    CBuffer vertexBuffer {
        m_context,
        vertexBufferSize,
        vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
        MemoryLocation::eDeviceOnly
    };

    CopyBuffer(
        m_context.Device(),
        commandPool,
        *stagingBuffer,
        *vertexBuffer,
        vertexBufferSize
    );

    const vk::DeviceSize indexBufferSize = sizeof(indices[0]) * indices.size();
    if (stagingBuffer.Size() < indexBufferSize) {
        stagingBuffer = CBuffer {
            m_context,
            indexBufferSize,
            vk::BufferUsageFlagBits::eTransferSrc,
            MemoryLocation::eHostVisible
        };
    }

    m_vertexBuffer = m_bufferManager.ImportBuffer("vertexBuffer", std::move(vertexBuffer));

    std::memcpy(stagingBuffer.Data(), indices.data(), indexBufferSize);

    CBuffer indexBuffer = {
        m_context,
        indexBufferSize,
        vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
        MemoryLocation::eDeviceOnly
    };

    CopyBuffer(
        m_context.Device(),
        commandPool,
        *stagingBuffer,
        *indexBuffer,
        indexBufferSize
    );

    m_indexBuffer = m_bufferManager.ImportBuffer("indexBuffer", std::move(indexBuffer));
}
}
