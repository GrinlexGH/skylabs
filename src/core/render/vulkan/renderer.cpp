#include "renderer.hpp"

#include "logging.hpp"
#include "shader.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <SDL3_image/SDL_image.h>

#include <chrono>
#include <ranges>

struct Vertex
{
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;

    constexpr static auto GetBindingDescription() -> vk::VertexInputBindingDescription {
        vk::VertexInputBindingDescription bindingDescription {};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = vk::VertexInputRate::eVertex;

        return bindingDescription;
    }

    constexpr static auto GetAttributeDescriptions() -> std::array<vk::VertexInputAttributeDescription, 3> {
        std::array<vk::VertexInputAttributeDescription, 3> attributeDescriptions{};

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = vk::Format::eR32G32B32Sfloat;
        attributeDescriptions[0].offset = offsetof(Vertex, pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = vk::Format::eR32G32B32Sfloat;
        attributeDescriptions[1].offset = offsetof(Vertex, color);

        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = vk::Format::eR32G32Sfloat;
        attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

        return attributeDescriptions;
    }
};

std::array vertices {
    Vertex { { -0.5f, -0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f } },
    Vertex { {  0.5f, -0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f } },
    Vertex { {  0.5f,  0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f } },
    Vertex { { -0.5f,  0.5f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f } },

    Vertex { { -0.5f, -0.5f, 0.5f }, { 1.0f, 1.0f, 0.0f }, { 0.0f, 0.0f } },
    Vertex { {  0.5f, -0.5f, 0.5f }, { 0.0f, 1.0f, 1.0f }, { 0.0f, 1.0f } },
    Vertex { {  0.5f,  0.5f, 0.5f }, { 1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } },
    Vertex { { -0.5f,  0.5f, 0.5f }, { 0.5f, 0.5f, 0.5f }, { 1.0f, 0.0f } },
};

struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
    glm::vec3 offset;
};

glm::vec3 offset = {0.0f, 0.0f, 0.0f}; // то что реально уходит в UBO
glm::vec3 targetOffset  = {0.0f, 0.0f, 0.0f}; // то куда хотим прийти

float lerpSpeed = 5.0f; // чем больше, тем быстрее двигается

void MoveForward()  { targetOffset.z += 0.1f; }
void MoveBackward() { targetOffset.z -= 0.1f; }

const std::array<std::uint16_t, 12> indices = {
    0, 1, 2, 2, 3, 0,   // первый квадрат
    4, 5, 6, 6, 7, 4    // второй квадрат
};

namespace {
auto CreateFrameBuffers(
    const vk::raii::Device& device,
    const Vulkan::CSwapchain& swapchain,
    const vk::raii::RenderPass& renderPass,
    const vk::raii::ImageView& depthImageView
) -> std::vector<vk::raii::Framebuffer> {
    std::vector<vk::raii::Framebuffer> out;

    out.reserve(swapchain.GetImageCount());
    for (const auto& imageView : swapchain.GetImageViews()) {
        std::array attachments = {
            *imageView,
            *depthImageView
        };

        vk::FramebufferCreateInfo framebufferInfo {};
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = swapchain.GetExtent().width;
        framebufferInfo.height = swapchain.GetExtent().height;
        framebufferInfo.layers = 1;

        out.emplace_back(device, framebufferInfo);
    }

    return out;
}

auto Resize(
    const vk::raii::Device& device,
    const vk::raii::RenderPass& renderPass,
    const vk::raii::ImageView& depthImageView,
    Vulkan::CSwapchain& swapchain,
    std::vector<vk::raii::Framebuffer>& frameBuffers
) -> void {
    swapchain.Recreate();
    frameBuffers = CreateFrameBuffers(device, swapchain, renderPass, depthImageView);
}

auto BeginSingleTimeCommands(
    const vk::raii::Device& device,
    const vk::CommandPool& commandPool
) -> vk::raii::CommandBuffer {
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

auto EndSingleTimeCommands(
    const Vulkan::CDevice& device,
    vk::raii::CommandBuffer& commandBuffer
) -> void {
    commandBuffer.end();

    vk::SubmitInfo submitInfo {};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &*commandBuffer;

    device.GetGraphicsQueue().m_handle.submit(submitInfo);
    device.GetGraphicsQueue().m_handle.waitIdle(); // TODO: USE FENCES

    commandBuffer.clear();
}

auto FindMemoryType(
    const vk::raii::PhysicalDevice& physicalDevice,
    const std::uint32_t typeFilter,
    vk::MemoryPropertyFlags properties
) -> std::uint32_t {
    const static vk::PhysicalDeviceMemoryProperties memProperties = physicalDevice.getMemoryProperties();

    for (std::uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("Failed to find suitable memory type!");
}

auto CreateBuffer(
    const vk::raii::PhysicalDevice& physicalDevice,
    const vk::raii::Device& device,
    vk::DeviceSize size,
    vk::BufferUsageFlags usage,
    vk::MemoryPropertyFlags properties,
    vk::raii::Buffer& buffer,
    vk::raii::DeviceMemory& bufferMemory
) -> void {
    vk::BufferCreateInfo bufferInfo {};
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = vk::SharingMode::eExclusive;

    buffer = vk::raii::Buffer { device, bufferInfo };

    vk::MemoryRequirements memRequirements =  buffer.getMemoryRequirements();

    vk::MemoryAllocateInfo allocInfo {};
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(
        physicalDevice,
        memRequirements.memoryTypeBits,
        properties
    );

    bufferMemory = vk::raii::DeviceMemory { device, allocInfo };
    buffer.bindMemory(bufferMemory, 0);
}

void CopyBuffer(
    const Vulkan::CDevice& device,
    const vk::raii::CommandPool& commandPool,
    vk::Buffer srcBuffer,
    vk::Buffer dstBuffer,
    vk::DeviceSize size
) {
    vk::raii::CommandBuffer commandBuffer = BeginSingleTimeCommands(device.GetHandle(), commandPool);

    vk::BufferCopy copyRegion;
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size = size;

    commandBuffer.copyBuffer(srcBuffer, dstBuffer, copyRegion);

    EndSingleTimeCommands(device, commandBuffer);
}

void UpdateUniformBuffer(
    const vk::Extent2D& cameraDemensions,
    std::vector<void*>& uniformBuffersMapped,
    std::uint32_t currentImage,
    glm::mat4 view,
    float deltaTime
) {
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::hours::period>(currentTime - startTime).count();
    (void)time;
    UniformBufferObject ubo {};
    ubo.model = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    ubo.view = view;
    ubo.proj = glm::perspective(glm::radians(90.0f), (float) cameraDemensions.width / (float) cameraDemensions.height, 0.01f, 10.0f);
    ubo.proj[1][1] *= -1;
    offset = glm::mix(offset, targetOffset, lerpSpeed * deltaTime);
    if (glm::length(targetOffset - offset) < 0.0001) {
        offset = targetOffset;
    }
    ubo.offset = offset;

    std::memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
}
}

namespace Vulkan {
CRenderer::CRenderer(const IWindow* const window) {
    if (window == nullptr) {
        throw std::runtime_error("Cannot initialize vulkan renderer. Window is nullptr");
    }
    m_context = CContext { window };

    m_surface = CSurface { &m_context };

    m_swapchain = CSwapchain { &m_context, m_surface.GetHandle(), 3, vk::PresentModeKHR::eImmediate };

    // Слава богу c++23 слава комитету слава ISO IEC
    m_frameData.reserve(FRAMES_IN_FLIGHT_COUNT);
    std::generate_n(std::back_inserter(m_frameData), FRAMES_IN_FLIGHT_COUNT, [&] {
        return CFrameData(&m_context);
    });

    const std::uint32_t imageCount = m_swapchain.GetImageCount();
    m_renderFinishedSemaphores.reserve(imageCount);
    std::generate_n(std::back_inserter(m_renderFinishedSemaphores), imageCount, [&] {
        return vk::raii::Semaphore { m_context.GetDevice().GetHandle(), vk::SemaphoreCreateInfo {} };
    });

    //region Subpasses
    // Создаём аттачмент - описание того, что сделать с картинкой.

    //region Depth Stencil
    //region Format
    auto findSupportedFormat = [this](const std::vector<vk::Format>& candidates, const vk::ImageTiling& tiling, const vk::FormatFeatureFlags& features) -> vk::Format {
        for (const vk::Format format : candidates) {
            const vk::FormatProperties props = m_context.GetPhysicalDevice()->GetHandle().getFormatProperties(format);

            if (tiling == vk::ImageTiling::eLinear && (props.linearTilingFeatures & features) == features) {
                return format;
            }
            if (tiling == vk::ImageTiling::eOptimal && (props.optimalTilingFeatures & features) == features) {
                return format;
            }
        }

        throw std::runtime_error("failed to find supported format!");
    };

    auto findDepthFormat = [&] -> vk::Format {
        return findSupportedFormat(
        { vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD32SfloatS8Uint },
            vk::ImageTiling::eOptimal,
            vk::FormatFeatureFlagBits::eDepthStencilAttachment
        );
    };
    //endregion Format

    m_depthBuffer = CImage {
        &m_context,
        { m_swapchain.GetExtent().width, m_swapchain.GetExtent().height, 1 },
        findDepthFormat(),
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eDepthStencilAttachment,
        vk::ImageAspectFlagBits::eDepth,
        vk::MemoryPropertyFlagBits::eDeviceLocal
    };

    vk::AttachmentDescription depthAttachment {};
    depthAttachment.format = findDepthFormat();
    depthAttachment.samples = vk::SampleCountFlagBits::e1;
    depthAttachment.loadOp = vk::AttachmentLoadOp::eClear;
    depthAttachment.storeOp = vk::AttachmentStoreOp::eDontCare;
    depthAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    depthAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    depthAttachment.initialLayout = vk::ImageLayout::eUndefined;
    depthAttachment.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

    vk::AttachmentReference depthAttachmentRef {};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
    //endregion Depth Stencil

    // Этот аттачмент будем использовать как colorAttachment,
    // то есть шейдер в него будет писать
    vk::AttachmentDescription colorAttachment {};
    colorAttachment.format = m_swapchain.GetSurfaceFormat().format;
    colorAttachment.samples = vk::SampleCountFlagBits::e1;
    colorAttachment.loadOp = vk::AttachmentLoadOp::eDontCare;
    colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
    colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    colorAttachment.initialLayout = vk::ImageLayout::eUndefined;
    colorAttachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;

    // Аттачмнет референс, который описывает просто layout аттачмента
    vk::AttachmentReference colorAttachmentRef {};
    colorAttachmentRef.attachment = 0; // Индекс в vk::SubpassDescription
    colorAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

    // Описание сабпасса
    vk::SubpassDescription subpass {};
    subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
    subpass.inputAttachmentCount = 0;
    subpass.pInputAttachments = nullptr;
    // Аттачмент в который будем писать
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.preserveAttachmentCount = 0;
    subpass.pPreserveAttachments = nullptr;
    subpass.pResolveAttachments = nullptr;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    // Описываем как сабпассы будут связаны
    vk::SubpassDependency dependency {};
    dependency.srcSubpass = vk::SubpassExternal; // Пустой внешний сабпасс
    dependency.dstSubpass = 0; // Описание применяется к первому (нулевому) сабпассу
    dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput; // Ждём когда на этой стадии закончатся операции
    dependency.srcAccessMask = {}; // Какие конкретно операции. Пустое значит все операции
    dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput; // На какую стадию идём
    dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite; // Что делаем

    std::array attachments = { colorAttachment, depthAttachment };
    vk::RenderPassCreateInfo renderPassInfo {};
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;
    m_renderPass = m_context.GetDevice().GetHandle().createRenderPass(renderPassInfo);
    //endregion Subpasses

    //region Shaders
    const CShader vertexShader(&m_context, CShader::Type::eVertex, "shader.vert.spv");
    const CShader fragmentShader(&m_context, CShader::Type::eFragment, "shader.frag.spv");

    const std::array shaderStages = {
        vertexShader.GetPipelineShaderCreateInfo(),
        fragmentShader.GetPipelineShaderCreateInfo(),
    };
    //endregion Shaders

    //region TEXTURE
    (void)(0);
    //region LOAD TEXTURE
    SDL_Surface* imageRaw = IMG_Load("assets/texture.jpg");
    if (!imageRaw) {
        throw std::runtime_error("failed to load texture image!");
    }
    SDL_Surface* image = SDL_ConvertSurface(imageRaw, SDL_PIXELFORMAT_ABGR8888);
    SDL_DestroySurface(imageRaw);
    vk::DeviceSize imageSize = static_cast<vk::DeviceSize>(image->w) * image->h * 4;
    vk::raii::Buffer stagingBuffer = nullptr;
    vk::raii::DeviceMemory stagingBufferMemory = nullptr;
    void* data;
    CreateBuffer(
        m_context.GetPhysicalDevice()->GetHandle(),
        m_context.GetDevice().GetHandle(),
        imageSize,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
        stagingBuffer,
        stagingBufferMemory
    );

    int b = imageSize / 3;
    char* a = new char[b];
    data = nullptr;
    data = stagingBufferMemory.mapMemory(0, imageSize);
    std::memcpy(data, image->pixels, static_cast<size_t>(imageSize / 1));
    std::memcpy((char*)data + (3 * imageSize / 4), a, imageSize / 4);
    stagingBufferMemory.unmapMemory();
    delete[] a;

    //endregion LOAD TEXTURE

    //region COPY TO IMAGE
    m_texture = CImage {
        &m_context,
        { static_cast<uint32_t>(image->w), static_cast<uint32_t>(image->h), 1 },
        vk::Format::eR8G8B8A8Srgb,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
        vk::ImageAspectFlagBits::eColor,
        vk::MemoryPropertyFlagBits::eDeviceLocal
    };

    auto commandPool = m_context.GetDevice().GetHandle().createCommandPool({
            vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
            m_context.GetDevice().GetGraphicsQueue().m_familyIndex
    });

    vk::raii::CommandBuffer commandBuffer = BeginSingleTimeCommands(m_context.GetDevice().GetHandle(), commandPool);

    m_texture.TransitionLayout(commandBuffer, vk::ImageLayout::eTransferDstOptimal);
    m_texture.CopyBufferToImage(commandBuffer, stagingBuffer, { static_cast<uint32_t>(image->w), static_cast<uint32_t>(image->h), 1 });
    m_texture.TransitionLayout(commandBuffer, vk::ImageLayout::eShaderReadOnlyOptimal);

    EndSingleTimeCommands(m_context.GetDevice(), commandBuffer);


    stagingBuffer.clear();
    stagingBufferMemory.clear();
    SDL_DestroySurface(image);
    image = nullptr;
    //endregion COPY TO IMAGE

    //endregion TEXTURE


    //region SAMPLER
    vk::SamplerCreateInfo samplerInfo {};
    samplerInfo.magFilter = vk::Filter::eNearest;
    samplerInfo.minFilter = vk::Filter::eNearest;
    samplerInfo.addressModeU = vk::SamplerAddressMode::eRepeat;
    samplerInfo.addressModeV = vk::SamplerAddressMode::eRepeat;
    samplerInfo.addressModeW = vk::SamplerAddressMode::eRepeat;
    samplerInfo.anisotropyEnable = vk::True;
    samplerInfo.maxAnisotropy = m_context.GetPhysicalDevice()->GetProperties().limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = vk::BorderColor::eIntOpaqueBlack;
    samplerInfo.unnormalizedCoordinates = vk::False;
    samplerInfo.compareEnable = vk::False;
    samplerInfo.compareOp = vk::CompareOp::eAlways;
    samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;
    m_textureSampler = vk::raii::Sampler { m_context.GetDevice().GetHandle(), samplerInfo };
    //endregion SAMPLER

    //region UBO
    vk::DeviceSize bufferSize = sizeof(UniformBufferObject);

    m_uniformBuffers.reserve(FRAMES_IN_FLIGHT_COUNT);
    m_uniformBuffersMemory.reserve(FRAMES_IN_FLIGHT_COUNT);
    m_uniformBuffersMapped.reserve(FRAMES_IN_FLIGHT_COUNT);

    for (size_t i = 0; i < FRAMES_IN_FLIGHT_COUNT; i++) {
        vk::raii::Buffer buffer { nullptr };
        vk::raii::DeviceMemory bufferMemory { nullptr };
        CreateBuffer(
            m_context.GetPhysicalDevice()->GetHandle(),
            m_context.GetDevice().GetHandle(),
            bufferSize,
            vk::BufferUsageFlagBits::eUniformBuffer,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
            buffer,
            bufferMemory
        );
        m_uniformBuffers.emplace_back(std::move(buffer));
        m_uniformBuffersMemory.emplace_back(std::move(bufferMemory));
        m_uniformBuffersMapped.emplace_back(m_uniformBuffersMemory[i].mapMemory(0, bufferSize));
    }
    //endregion UBO

    //region DESCRIPTOR SETS
    //region DESCRIPTOR SET LAYOUT
    vk::DescriptorSetLayoutBinding uboLayoutBinding {};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eVertex;
    uboLayoutBinding.pImmutableSamplers = nullptr; // Optional

    vk::DescriptorSetLayoutBinding samplerLayoutBinding {};
    samplerLayoutBinding.binding = 1;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    samplerLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;

    std::array bindings = { uboLayoutBinding, samplerLayoutBinding };

    vk::DescriptorSetLayoutCreateInfo layoutInfo {};
    layoutInfo.bindingCount = static_cast<std::uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    m_descriptorSetLayout = vk::raii::DescriptorSetLayout { m_context.GetDevice().GetHandle(), layoutInfo };
    //endregion DESCRIPTOR SET LAYOUT

    //region DESCRIPTOR POOL
    std::array<vk::DescriptorPoolSize, 2> poolSizes {};
    poolSizes[0].type = vk::DescriptorType::eUniformBuffer;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(FRAMES_IN_FLIGHT_COUNT);
    poolSizes[1].type = vk::DescriptorType::eCombinedImageSampler;
    poolSizes[1].descriptorCount = static_cast<uint32_t>(FRAMES_IN_FLIGHT_COUNT);

    vk::DescriptorPoolCreateInfo poolInfo {};
    poolInfo.poolSizeCount = static_cast<std::uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(FRAMES_IN_FLIGHT_COUNT);

    m_descriptorPool = vk::raii::DescriptorPool { m_context.GetDevice().GetHandle(), poolInfo };
    //endregion DESCRIPTOR POOL

    //region DESCRIPTOR SETS
    std::vector<vk::DescriptorSetLayout> layouts(FRAMES_IN_FLIGHT_COUNT, m_descriptorSetLayout);
    vk::DescriptorSetAllocateInfo descriptorAllocInfo {};
    descriptorAllocInfo.descriptorPool = m_descriptorPool;
    descriptorAllocInfo.descriptorSetCount = static_cast<uint32_t>(FRAMES_IN_FLIGHT_COUNT);
    descriptorAllocInfo.pSetLayouts = layouts.data();

    m_descriptorSets = (*m_context.GetDevice().GetHandle()).allocateDescriptorSets(descriptorAllocInfo);

    for (size_t i = 0; i < FRAMES_IN_FLIGHT_COUNT; i++) {
        vk::DescriptorBufferInfo bufferInfo {};
        bufferInfo.buffer = m_uniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        vk::DescriptorImageInfo imageInfo {};
        imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        imageInfo.imageView = m_texture.GetView();
        imageInfo.sampler = m_textureSampler;

        std::array<vk::WriteDescriptorSet, 2> descriptorWrites{};
        descriptorWrites[0].dstSet = m_descriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = vk::DescriptorType::eUniformBuffer;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfo;
        descriptorWrites[0].pImageInfo = nullptr;
        descriptorWrites[0].pTexelBufferView = nullptr;

        descriptorWrites[1].dstSet = m_descriptorSets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = vk::DescriptorType::eCombinedImageSampler;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pImageInfo = &imageInfo;

        m_context.GetDevice().GetHandle().updateDescriptorSets(descriptorWrites, {});
    }
    //endregion DESCRIPTOR SET
    //endregion DESCRIPTOR SETS

    //region PIPELINE
    constexpr vk::VertexInputBindingDescription bindingDescription = Vertex::GetBindingDescription();
    constexpr std::array<vk::VertexInputAttributeDescription, 3> attributeDescriptions = Vertex::GetAttributeDescriptions();

    vk::PipelineVertexInputStateCreateInfo vertexInputInfo {};
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    vk::PipelineInputAssemblyStateCreateInfo inputAssembly {};
    inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;
    inputAssembly.primitiveRestartEnable = vk::False;

    vk::PipelineViewportStateCreateInfo viewportState {};
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    vk::PipelineRasterizationStateCreateInfo rasterizer {};
    rasterizer.depthClampEnable = vk::False;
    rasterizer.rasterizerDiscardEnable = vk::False;
    rasterizer.polygonMode = vk::PolygonMode::eFill;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = vk::CullModeFlagBits::eNone;
    rasterizer.frontFace = vk::FrontFace::eClockwise;
    rasterizer.depthBiasEnable = vk::False;

    vk::PipelineMultisampleStateCreateInfo multisampling {};
    multisampling.sampleShadingEnable = vk::False;
    multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;

    vk::PipelineColorBlendAttachmentState colorBlendAttachment {};
    colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
    colorBlendAttachment.blendEnable = vk::False;

    vk::PipelineColorBlendStateCreateInfo colorBlending {};
    colorBlending.logicOpEnable = vk::False;
    colorBlending.logicOp = vk::LogicOp::eCopy;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    constexpr std::array dynamicStates = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor,
    };
    vk::PipelineDynamicStateCreateInfo dynamicState {};
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    vk::PipelineDepthStencilStateCreateInfo depthStencil {};
    depthStencil.depthTestEnable = vk::True;
    depthStencil.depthWriteEnable = vk::True;
    depthStencil.depthCompareOp = vk::CompareOp::eLess;
    depthStencil.depthBoundsTestEnable = vk::False;
    depthStencil.stencilTestEnable = vk::False;

    vk::PipelineLayoutCreateInfo pipelineLayoutInfo {};
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &*m_descriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 0;

    m_pipelineLayout = m_context.GetDevice().GetHandle().createPipelineLayout(pipelineLayoutInfo);

    vk::GraphicsPipelineCreateInfo pipelineInfo {};
    pipelineInfo.stageCount = shaderStages.size();
    pipelineInfo.pStages = shaderStages.data();
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.layout = m_pipelineLayout;
    pipelineInfo.renderPass = m_renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    m_pipeline = m_context.GetDevice().GetHandle().createGraphicsPipeline(nullptr, pipelineInfo);
    //endregion PIPELINE

    m_frameBuffers = CreateFrameBuffers(m_context.GetDevice().GetHandle(), m_swapchain, m_renderPass, m_depthBuffer.GetView());

    //region VERTEX BUFFER
    vk::DeviceSize vertexBufferSize = sizeof(vertices[0]) * vertices.size();

    CreateBuffer(
        m_context.GetPhysicalDevice()->GetHandle(),
        m_context.GetDevice().GetHandle(),
        vertexBufferSize,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
        stagingBuffer,
        stagingBufferMemory
    );

    data = stagingBufferMemory.mapMemory(0, vertexBufferSize);
        memcpy(data, vertices.data(), vertexBufferSize);
    stagingBufferMemory.unmapMemory();

    CreateBuffer(
        m_context.GetPhysicalDevice()->GetHandle(),
        m_context.GetDevice().GetHandle(),
        vertexBufferSize,
        vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        m_vertexBuffer,
        m_vertexBufferMemory
    );

    CopyBuffer(
        m_context.GetDevice(),
        commandPool,
        stagingBuffer,
        m_vertexBuffer,
        vertexBufferSize
    );

    stagingBuffer.clear();
    stagingBufferMemory.clear();
    //endregion VERTEX BUFFER

    //region INDEX BUFFER
    vk::DeviceSize indexBufferSize = sizeof(indices[0]) * indices.size();

    CreateBuffer(
        m_context.GetPhysicalDevice()->GetHandle(),
        m_context.GetDevice().GetHandle(),
        indexBufferSize,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
        stagingBuffer,
        stagingBufferMemory
    );

    data = stagingBufferMemory.mapMemory(0, indexBufferSize);
        memcpy(data, indices.data(), indexBufferSize);
    stagingBufferMemory.unmapMemory();

    CreateBuffer(
        m_context.GetPhysicalDevice()->GetHandle(),
        m_context.GetDevice().GetHandle(),
        indexBufferSize,
        vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        m_indexBuffer,
        m_indexBufferMemory
    );

    CopyBuffer(
        m_context.GetDevice(),
        commandPool,
        stagingBuffer,
        m_indexBuffer,
        indexBufferSize
    );

    stagingBuffer.clear();
    stagingBufferMemory.clear();
    //endregion INDEX BUFFER
}

std::unique_ptr<CRenderer> CRenderer::TryToCreate(const Vulkan::IWindow* const window) {
    try {
        return std::make_unique<CRenderer>(window);
    } catch (const std::exception& e) {
        Log::Error("Cannot initialize vulkan renderer: {}", e.what());
        return nullptr;
    }
}

void CRenderer::Draw(glm::mat4 view, float deltaTime) {
    const Vulkan::CFrameData& frameData = m_frameData[m_frameIndex];
    const vk::raii::Device& deviceHandle = m_context.GetDevice().GetHandle();
    const vk::raii::SwapchainKHR& swapchainHandle = m_swapchain.GetHandle();


    UpdateUniformBuffer(m_swapchain.GetExtent(), m_uniformBuffersMapped, m_frameIndex, view, deltaTime);

    // #region ACQUIRE_IMAGE
    vk::Result result = deviceHandle.waitForFences({ frameData.GetFence() }, vk::True, std::numeric_limits<std::uint64_t>::max());
    if (result != vk::Result::eSuccess) {
        throw std::runtime_error("Failed to wait for fence: " + vk::to_string(result));
    }

    std::uint32_t imageIndex;
    std::tie(result, imageIndex) = swapchainHandle.acquireNextImage(
        std::numeric_limits<std::uint64_t>::max(),
        *frameData.GetImageAvailableSemaphore(),
        VK_NULL_HANDLE
    );

    // Reset fence before we can return from function to avoid deadlock
    deviceHandle.resetFences({ frameData.GetFence() });

    if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR) {
        Resize(m_context.GetDevice().GetHandle(), m_renderPass, m_depthBuffer.GetView(), m_swapchain, m_frameBuffers);
        return;
    }
    if (result != vk::Result::eSuccess) {
        throw std::runtime_error("Failed to acquire swapchain image: " + vk::to_string(result));
    }
    // #endregion ACQUIRE_IMAGE


    // #region COMMAND_RECORD
    frameData.GetCommandBuffers()[0].reset();
    vk::CommandBufferBeginInfo beginInfo {};
    frameData.GetCommandBuffers()[0].begin(beginInfo);

    // #region RENDER_PASS_BEGIN
    vk::RenderPassBeginInfo renderPassInfo {};
    renderPassInfo.renderPass = m_renderPass;
    renderPassInfo.framebuffer = m_frameBuffers[imageIndex];
    renderPassInfo.renderArea.offset = { { 0, 0 } };
    renderPassInfo.renderArea.extent = m_swapchain.GetExtent();

    std::array<vk::ClearValue, 2> clearValues{};
    clearValues[0].color = vk::ClearColorValue { 0.2f, 0.3f, 0.6f, 1.0f };
    clearValues[1].depthStencil = vk::ClearDepthStencilValue { 1.0f, 0 };

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    frameData.GetCommandBuffers()[0].beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
    // #endregion RENDER_PASS_BEGIN

    frameData.GetCommandBuffers()[0].bindPipeline(vk::PipelineBindPoint::eGraphics, m_pipeline);

    vk::Viewport viewport {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(m_swapchain.GetExtent().width);
    viewport.height = static_cast<float>(m_swapchain.GetExtent().height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    frameData.GetCommandBuffers()[0].setViewport(0, viewport);

    vk::Rect2D scissor {};
    scissor.offset = { { 0, 0 } };
    scissor.extent = m_swapchain.GetExtent();
    frameData.GetCommandBuffers()[0].setScissor(0, scissor);

    vk::Buffer vertexBuffers[] = { m_vertexBuffer };
    vk::DeviceSize offsets[] = { 0 };
    frameData.GetCommandBuffers()[0].bindVertexBuffers(0, vertexBuffers, offsets);
    frameData.GetCommandBuffers()[0].bindIndexBuffer(m_indexBuffer, 0, vk::IndexType::eUint16);

    frameData.GetCommandBuffers()[0].bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics,
        m_pipelineLayout,
        0,
        m_descriptorSets[m_frameIndex],
        {}
    );

    frameData.GetCommandBuffers()[0].drawIndexed(static_cast<std::uint32_t>(indices.size()), 1, 0, 0, 0);

    frameData.GetCommandBuffers()[0].endRenderPass();
    frameData.GetCommandBuffers()[0].end();
    // #endregion COMMAND_RECORD

    // #region COMMAND_SUBMIT
    vk::SubmitInfo submitInfo {};

    vk::PipelineStageFlags waitStages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &*frameData.GetImageAvailableSemaphore();
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &*frameData.GetCommandBuffers()[0];

    vk::Semaphore signalSemaphores[] = { m_renderFinishedSemaphores[imageIndex] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    m_context.GetDevice().GetGraphicsQueue().m_handle.submit(submitInfo, frameData.GetFence());
    // #endregion COMMAND_RECORD

    // #region PRESENT
    vk::PresentInfoKHR presentInfo {};

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &*swapchainHandle;

    presentInfo.pImageIndices = &imageIndex;

    result = m_context.GetDevice().GetPresentQueue().m_handle.presentKHR(presentInfo);
    if (result == vk::Result::eErrorOutOfDateKHR) {
        Resize(m_context.GetDevice().GetHandle(), m_renderPass, m_depthBuffer.GetView(), m_swapchain, m_frameBuffers);
        return;
    }
    // #endregion PRESENT

    m_frameIndex = (m_frameIndex + 1) % FRAMES_IN_FLIGHT_COUNT;
}

CRenderer::~CRenderer() {
    m_context.GetDevice().GetHandle().waitIdle();
}
}
