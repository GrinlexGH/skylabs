#include "vulkan_renderer.hpp"

#include "logging.hpp"
#include "resource_system.hpp"

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <SDL3_image/SDL_image.h>

#include <chrono>

constexpr int MAX_FRAMES_IN_FLIGHT = 25;

struct Vertex
{
    glm::vec2 pos;
    glm::vec3 color;

    static vk::VertexInputBindingDescription getBindingDescription() {
        vk::VertexInputBindingDescription bindingDescription {};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = vk::VertexInputRate::eVertex;

        return bindingDescription;
    }

    static std::array<vk::VertexInputAttributeDescription, 2> getAttributeDescriptions() {
        std::array<vk::VertexInputAttributeDescription, 2> attributeDescriptions {};
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = vk::Format::eR32G32Sfloat;
        attributeDescriptions[0].offset = offsetof(Vertex, pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = vk::Format::eR32G32B32Sfloat;
        attributeDescriptions[1].offset = offsetof(Vertex, color);

        return attributeDescriptions;
    }
};

struct UniformBufferObject {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};

const std::array vertices {
    Vertex { .pos = {-0.5f, -0.5f}, .color = {1.0f, 0.0f, 0.0f}},
    Vertex { .pos = {0.5f, -0.5f}, .color = {0.0f, 1.0f, 0.0f}},
    Vertex { .pos = {0.5f, 0.5f}, .color = {0.0f, 0.0f, 1.0f}},
    Vertex { .pos = { -0.5f, 0.5f }, .color = { 1.0f, 1.0f, 1.0f } },
};

const std::array<std::uint16_t, 6> indices = {
    0, 1, 2,
    2, 3, 0
};

namespace {
std::vector<vk::Framebuffer> CreateFrameBuffers(
    const vk::Device& deviceHandle,
    const Vulkan::CSwapchain* swapchain,
    const vk::RenderPass& renderPass
) {
    const Vulkan::CSwapchain::CInfo& swapchainInfo = swapchain->GetInfo();

    std::vector<vk::Framebuffer> out;
    out.reserve(swapchainInfo.m_imageCount);
    for (std::size_t i = 0; i < swapchainInfo.m_imageCount; i++) {
        vk::FramebufferCreateInfo framebufferInfo {};
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = &swapchain->GetImageViews()[i];
        framebufferInfo.width = swapchainInfo.m_extent.width;
        framebufferInfo.height = swapchainInfo.m_extent.height;
        framebufferInfo.layers = 1;

        out.emplace_back(deviceHandle.createFramebuffer(framebufferInfo));
    }

    return out;
}

void DestroyFramebuffers(const vk::Device& deviceHandle, std::vector<vk::Framebuffer>& frameBuffers) {
    for (const auto& framebuffer : frameBuffers) {
        deviceHandle.destroyFramebuffer(framebuffer);
    }
    frameBuffers.clear();
}

void Resize(
    const vk::Device& deviceHandle,
    Vulkan::CSwapchain* swapchain,
    std::vector<vk::Framebuffer>& frameBuffers,
    const vk::RenderPass& renderPass
) {
    swapchain->Recreate();
    DestroyFramebuffers(deviceHandle, frameBuffers);
    frameBuffers = CreateFrameBuffers(deviceHandle, swapchain, renderPass);
}

vk::CommandBuffer BeginSingleTimeCommands(
    vk::Device deviceHandle,
    vk::CommandPool commandPool
) {
    vk::CommandBufferAllocateInfo allocInfo {};
    allocInfo.level = vk::CommandBufferLevel::ePrimary;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    vk::CommandBuffer commandBuffer = deviceHandle.allocateCommandBuffers(allocInfo)[0];

    vk::CommandBufferBeginInfo beginInfo {};
    beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

    commandBuffer.begin(beginInfo);

    return commandBuffer;
}

void EndSingleTimeCommands(
    vk::Device deviceHandle,
    vk::Queue graphicsQueueHandle,
    vk::CommandPool commandPool,
    vk::CommandBuffer commandBuffer
) {
    commandBuffer.end();

    vk::SubmitInfo submitInfo {};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    graphicsQueueHandle.submit(submitInfo);
    graphicsQueueHandle.waitIdle(); // TODO: USE FENCES

    deviceHandle.freeCommandBuffers(commandPool, commandBuffer);
}

std::uint32_t FindMemoryType(
    const vk::PhysicalDevice& physicalDeviceHandle,
    const std::uint32_t typeFilter,
    vk::MemoryPropertyFlags properties
) {
    static vk::PhysicalDeviceMemoryProperties memProperties = physicalDeviceHandle.getMemoryProperties();

    for (std::uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");
}

void CreateBuffer(
    vk::PhysicalDevice physicalDeviceHandle,
    vk::Device deviceHandle,
    vk::DeviceSize size,
    vk::BufferUsageFlags usage,
    vk::MemoryPropertyFlags properties,
    vk::Buffer& buffer,
    vk::DeviceMemory& bufferMemory
) {
    vk::BufferCreateInfo bufferInfo {};
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = vk::SharingMode::eExclusive;

    buffer = deviceHandle.createBuffer(bufferInfo);

    vk::MemoryRequirements memRequirements = deviceHandle.getBufferMemoryRequirements(buffer);

    vk::MemoryAllocateInfo allocInfo {};
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(
        physicalDeviceHandle,
        memRequirements.memoryTypeBits,
        properties
    );

    bufferMemory = deviceHandle.allocateMemory(allocInfo);
    deviceHandle.bindBufferMemory(buffer, bufferMemory, 0);
}

void CopyBuffer(
    vk::Device deviceHandle,
    vk::Queue graphicsQueueHandle,
    vk::CommandPool commandPool,
    vk::Buffer srcBuffer,
    vk::Buffer dstBuffer,
    vk::DeviceSize size
) {
    vk::CommandBuffer commandBuffer = BeginSingleTimeCommands(deviceHandle, commandPool);

    vk::BufferCopy copyRegion {};
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size = size;

    commandBuffer.copyBuffer(srcBuffer, dstBuffer, copyRegion);

    EndSingleTimeCommands(deviceHandle, graphicsQueueHandle, commandPool, commandBuffer);
}

void CreateImage(
    const vk::PhysicalDevice& physicalDeviceHandle,
    const vk::Device& deviceHandle,
    std::uint32_t width,
    std::uint32_t height,
    vk::Format format,
    vk::ImageTiling tiling,
    vk::ImageUsageFlags usage,
    vk::MemoryPropertyFlags properties,
    vk::Image& texture,
    vk::DeviceMemory& textureMemory
) {
    vk::ImageCreateInfo imageInfo{};
    imageInfo.imageType = vk::ImageType::e2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = vk::ImageLayout::eUndefined;
    imageInfo.usage = usage;
    imageInfo.sharingMode = vk::SharingMode::eExclusive;
    imageInfo.samples = vk::SampleCountFlagBits::e1;
    imageInfo.flags = {};

    texture = deviceHandle.createImage(imageInfo);

    vk::MemoryRequirements memRequirements;
    memRequirements = deviceHandle.getImageMemoryRequirements(texture);

    vk::MemoryAllocateInfo allocInfo {};
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(
        physicalDeviceHandle,
        memRequirements.memoryTypeBits,
        properties
    );

    textureMemory = deviceHandle.allocateMemory(allocInfo);
    deviceHandle.bindImageMemory(texture, textureMemory, 0);
}

void CopyBufferToImage(
    vk::Device deviceHandle,
    vk::CommandPool commandPool,
    vk::Queue graphicsQueueHandle,
    vk::Buffer buffer,
    vk::Image image,
    uint32_t width,
    uint32_t height
) {
    vk::CommandBuffer commandBuffer = BeginSingleTimeCommands(deviceHandle, commandPool);

    vk::BufferImageCopy region {};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;

    region.imageOffset = vk::Offset3D {0, 0, 0};
    region.imageExtent = vk::Extent3D {
        width,
        height,
        1
    };

    commandBuffer.copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, region);

    EndSingleTimeCommands(deviceHandle, graphicsQueueHandle, commandPool, commandBuffer);
}

void TransitionImageLayout(
    vk::Device deviceHandle,
    vk::CommandPool commandPool,
    vk::Queue graphicsQueueHandle,
    vk::Image image,
    vk::Format format,
    vk::ImageLayout oldLayout,
    vk::ImageLayout newLayout
) {
    vk::CommandBuffer commandBuffer = BeginSingleTimeCommands(deviceHandle, commandPool);

    vk::ImageMemoryBarrier barrier{};   // todo: sync2
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
    barrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    vk::PipelineStageFlags sourceStage;
    vk::PipelineStageFlags destinationStage;

    if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal) {
        barrier.srcAccessMask = {};
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

        sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
        destinationStage = vk::PipelineStageFlagBits::eTransfer;
    } else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

        sourceStage = vk::PipelineStageFlagBits::eTransfer;
        destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
    } else {
        throw std::invalid_argument("unsupported layout transition!");
    }
    commandBuffer.pipelineBarrier(sourceStage, destinationStage, {}, {}, {}, barrier);

    EndSingleTimeCommands(deviceHandle, graphicsQueueHandle, commandPool, commandBuffer);
}

void UpdateUniformBuffer(
    const vk::Extent2D& cameraDemensions,
    std::vector<void*>& uniformBuffersMapped,
    std::uint32_t currentImage,
    glm::mat4 view
) {
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    UniformBufferObject ubo {};
    ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(1.0f, 1.0f, 1.0f));
    ubo.view = view;
    ubo.proj = glm::perspective(glm::radians(45.0f), (float) cameraDemensions.width / (float) cameraDemensions.height, 0.1f, 10.0f);
    ubo.proj[1][1] *= -1;

    std::memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
}

}

CVulkanRenderer::CVulkanRenderer(const IVulkanWindow* const window) {
    if (window == nullptr) {
        throw std::runtime_error("Cannot initialize vulkan renderer. Window is nullptr");
    }

    m_context = std::make_unique<Vulkan::CRenderContext>(window);

    m_surface = std::make_unique<Vulkan::CSurface>(m_context.get());

    m_swapchain = std::make_unique<Vulkan::CSwapchain>(
        m_context.get(), m_surface->GetHandle(),
        3, vk::PresentModeKHR::eImmediate
    );

    vk::Device deviceHandle = m_context->GetDevice()->GetHandle();
    vk::PhysicalDevice physicalDeviceHandle = m_context->GetPhysicalDevice()->GetHandle();

    // #region RENDER_PASS
    vk::AttachmentDescription colorAttachment {};
    colorAttachment.format = m_swapchain->GetInfo().m_surfaceFormat.format;
    colorAttachment.samples = vk::SampleCountFlagBits::e1;
    colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
    colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
    colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    colorAttachment.initialLayout = vk::ImageLayout::eUndefined;
    colorAttachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;

    vk::AttachmentReference colorAttachmentRef {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

    vk::SubpassDescription subpass {};
    subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    vk::SubpassDependency dependency {};
    dependency.srcSubpass = vk::SubpassExternal;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dependency.srcAccessMask = {};
    dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;

    vk::RenderPassCreateInfo renderPassInfo {};
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;
    m_renderPass = deviceHandle.createRenderPass(renderPassInfo);
    // #endregion RENDER_PASS

    // #region SHADER_MODULES
    const std::vector<char> vertexShaderSource = ResourceSystem::LoadShader("shader.vert.spv");
    vk::ShaderModuleCreateInfo createInfo {};
    createInfo.codeSize = vertexShaderSource.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(vertexShaderSource.data());

    const vk::ShaderModule vertShaderModule = deviceHandle.createShaderModule(createInfo);

    const std::vector<char> fragmentShaderSource = ResourceSystem::LoadShader("shader.frag.spv");
    createInfo.codeSize = fragmentShaderSource.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(fragmentShaderSource.data());

    const vk::ShaderModule fragShaderModule = deviceHandle.createShaderModule(createInfo);

    vk::PipelineShaderStageCreateInfo vertShaderStageInfo {};
    vertShaderStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    vk::PipelineShaderStageCreateInfo fragShaderStageInfo {};
    fragShaderStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    std::array shaderStages = { vertShaderStageInfo, fragShaderStageInfo };
    // #endregion SHADER_MODULES


    // #region UBO
    vk::DeviceSize bufferSize = sizeof(UniformBufferObject);

    m_uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    m_uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
    m_uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        CreateBuffer(
            physicalDeviceHandle,
            deviceHandle,
            bufferSize,
            vk::BufferUsageFlagBits::eUniformBuffer,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
            m_uniformBuffers[i],
            m_uniformBuffersMemory[i]
        );

        m_uniformBuffersMapped[i] = deviceHandle.mapMemory(m_uniformBuffersMemory[i], 0, bufferSize);
    }
    // #endregion UBO

    // #region DESCRIPTOR_SET_LAYOUT
    vk::DescriptorSetLayoutBinding uboLayoutBinding {};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eVertex;
    uboLayoutBinding.pImmutableSamplers = nullptr; // Optional

    vk::DescriptorSetLayoutCreateInfo layoutInfo {};
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &uboLayoutBinding;

    m_descriptorSetLayout = deviceHandle.createDescriptorSetLayout(layoutInfo);
    // #endregion DESCRIPTOR_SET_LAYOUT

    // #region DESCRIPTOR_POOL
    vk::DescriptorPoolSize poolSize {};
    poolSize.type = vk::DescriptorType::eUniformBuffer;
    poolSize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    vk::DescriptorPoolCreateInfo poolInfo {};
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    m_descriptorPool = deviceHandle.createDescriptorPool(poolInfo);
    // #endregion DESCRIPTOR_POOL

    // #region DESCRIPTOR_SETS
    std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, m_descriptorSetLayout);
    vk::DescriptorSetAllocateInfo descriptorAllocInfo {};
    descriptorAllocInfo.descriptorPool = m_descriptorPool;
    descriptorAllocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    descriptorAllocInfo.pSetLayouts = layouts.data();

    m_descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    m_descriptorSets = deviceHandle.allocateDescriptorSets(descriptorAllocInfo);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vk::DescriptorBufferInfo bufferInfo {};
        bufferInfo.buffer = m_uniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        vk::WriteDescriptorSet descriptorWrite {};
        descriptorWrite.dstSet = m_descriptorSets[i];
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo = &bufferInfo;
        descriptorWrite.pImageInfo = nullptr;
        descriptorWrite.pTexelBufferView = nullptr;

        deviceHandle.updateDescriptorSets(descriptorWrite, {});
    }
    // #endregion DESCRIPTOR_SET


    // #region PIPELINE
    vk::VertexInputBindingDescription bindingDescription = Vertex::getBindingDescription();
    std::array<vk::VertexInputAttributeDescription, 2> attributeDescriptions = Vertex::getAttributeDescriptions();

    vk::PipelineVertexInputStateCreateInfo vertexInputInfo {};
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<std::uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    vk::PipelineInputAssemblyStateCreateInfo inputAssembly {};
    inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;
    inputAssembly.primitiveRestartEnable = vk::False;

    vk::PipelineViewportStateCreateInfo viewportState {};
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    vk::PipelineRasterizationStateCreateInfo rasterizer {};
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = vk::PolygonMode::eFill;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = vk::CullModeFlagBits::eNone;
    rasterizer.frontFace = vk::FrontFace::eClockwise;
    rasterizer.depthBiasEnable = VK_FALSE;

    vk::PipelineMultisampleStateCreateInfo multisampling {};
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;

    vk::PipelineColorBlendAttachmentState colorBlendAttachment {};
    colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
    colorBlendAttachment.blendEnable = VK_FALSE;

    vk::PipelineColorBlendStateCreateInfo colorBlending {};
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = vk::LogicOp::eCopy;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    std::vector dynamicStates = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor
    };
    vk::PipelineDynamicStateCreateInfo dynamicState {};
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    vk::PipelineLayoutCreateInfo pipelineLayoutInfo {};
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &m_descriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 0;

    m_pipelineLayout = deviceHandle.createPipelineLayout(pipelineLayoutInfo);

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
    pipelineInfo.layout = m_pipelineLayout;
    pipelineInfo.renderPass = m_renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    m_pipeline = deviceHandle.createGraphicsPipeline(VK_NULL_HANDLE, pipelineInfo).value;
    // #endregion

    deviceHandle.destroyShaderModule(vertShaderModule);
    deviceHandle.destroyShaderModule(fragShaderModule);

    // #region COMMAND_BUFFERS
    vk::CommandPoolCreateInfo commandPoolInfo {};
    commandPoolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
    commandPoolInfo.queueFamilyIndex = m_context->GetDevice()->GetGraphicsQueue().m_familyIndex;

    m_commandPool = deviceHandle.createCommandPool(commandPoolInfo);

    m_commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    vk::CommandBufferAllocateInfo cmdBufferAllocInfo {};
    cmdBufferAllocInfo.commandPool = m_commandPool;
    cmdBufferAllocInfo.level = vk::CommandBufferLevel::ePrimary;
    cmdBufferAllocInfo.commandBufferCount = static_cast<std::uint32_t>(m_commandBuffers.size());
    m_commandBuffers = deviceHandle.allocateCommandBuffers(cmdBufferAllocInfo);
    // #endregion

    m_frameBuffers = CreateFrameBuffers(deviceHandle, m_swapchain.get(), m_renderPass);

    // #region SYNC_PRIMITIVES
    m_renderFinishedSemaphores.resize(m_swapchain->GetInfo().m_imageCount);
    for (unsigned int i = 0; i < m_swapchain->GetInfo().m_imageCount; ++i) {
        m_renderFinishedSemaphores[i] = deviceHandle.createSemaphore(vk::SemaphoreCreateInfo {});
    }

    m_currentImageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        m_currentImageAvailableSemaphores[i] = deviceHandle.createSemaphore(vk::SemaphoreCreateInfo {});
        m_inFlightFences[i] = deviceHandle.createFence(vk::FenceCreateInfo { vk::FenceCreateFlagBits::eSignaled });
    }
    // #endregion SYNC_PRIMITIVES

    // #region VERTEX_BUFFER
    vk::DeviceSize vertexBufferSize = sizeof(vertices[0]) * vertices.size();

    vk::Buffer stagingBuffer;
    vk::DeviceMemory stagingBufferMemory;
    CreateBuffer(
        physicalDeviceHandle,
        deviceHandle,
        vertexBufferSize,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
        stagingBuffer,
        stagingBufferMemory
    );

    void* data = deviceHandle.mapMemory(stagingBufferMemory, 0, vertexBufferSize);
        memcpy(data, vertices.data(), static_cast<std::size_t>(vertexBufferSize));
    deviceHandle.unmapMemory(stagingBufferMemory);

    CreateBuffer(
        physicalDeviceHandle,
        deviceHandle,
        vertexBufferSize,
        vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        m_vertexBuffer,
        m_vertexBufferMemory
    );

    CopyBuffer(
        deviceHandle,
        m_context->GetDevice()->GetGraphicsQueue().m_handle,
        m_commandPool,
        stagingBuffer,
        m_vertexBuffer,
        vertexBufferSize
    );

    deviceHandle.destroyBuffer(stagingBuffer);
    deviceHandle.freeMemory(stagingBufferMemory);
    // #endregion VERTEX_BUFFER

    // #region INDEX_BUFFER
    vk::DeviceSize indexBufferSize = sizeof(indices[0]) * indices.size();

    CreateBuffer(
        physicalDeviceHandle,
        deviceHandle,
        indexBufferSize,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
        stagingBuffer,
        stagingBufferMemory
    );

    data = deviceHandle.mapMemory(stagingBufferMemory, 0, indexBufferSize);
        memcpy(data, indices.data(), static_cast<std::size_t>(indexBufferSize));
    deviceHandle.unmapMemory(stagingBufferMemory);

    CreateBuffer(
        physicalDeviceHandle,
        deviceHandle,
        indexBufferSize,
        vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        m_indexBuffer,
        m_indexBufferMemory
    );

    CopyBuffer(
        deviceHandle,
        m_context->GetDevice()->GetGraphicsQueue().m_handle,
        m_commandPool,
        stagingBuffer,
        m_indexBuffer,
        indexBufferSize
    );

    deviceHandle.destroyBuffer(stagingBuffer);
    deviceHandle.freeMemory(stagingBufferMemory);
    // #endregion INDEX_BUFFER

    // #region TEXTURE
    // #region LOAD_TEXTURE
    SDL_Surface* image = IMG_Load("texture.jpg");
    if (!image) {
        throw std::runtime_error("failed to load texture image!");
    }
    SDL_ConvertSurface(image, SDL_PIXELFORMAT_RGBA8888);
    vk::DeviceSize imageSize = image->w * image->h * 4;

    CreateBuffer(
        physicalDeviceHandle,
        deviceHandle,
        imageSize,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
        stagingBuffer,
        stagingBufferMemory
    );

    data = nullptr;
    data = deviceHandle.mapMemory(stagingBufferMemory, 0, imageSize);
    std::memcpy(data, image->pixels, static_cast<size_t>(imageSize / 4));
    deviceHandle.unmapMemory(stagingBufferMemory);

    // #endregion LOAD_TEXTURE

    CreateImage(
        physicalDeviceHandle,
        deviceHandle,
        image->w,
        image->h,
        vk::Format::eR8G8B8A8Srgb,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        m_texture,
        m_textureMemory
    );

    TransitionImageLayout(
        deviceHandle,
        m_commandPool,
        m_context->GetDevice()->GetGraphicsQueue().m_handle,
        m_texture,
        vk::Format::eR8G8B8A8Srgb,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eTransferDstOptimal
    );
    CopyBufferToImage(
        deviceHandle,
        m_commandPool,
        m_context->GetDevice()->GetGraphicsQueue().m_handle,
        stagingBuffer, m_texture, static_cast<uint32_t>(image->w), static_cast<uint32_t>(image->h)
    );
    TransitionImageLayout(
        deviceHandle,
        m_commandPool,
        m_context->GetDevice()->GetGraphicsQueue().m_handle,
        m_texture,
        vk::Format::eR8G8B8A8Srgb,
        vk::ImageLayout::eTransferDstOptimal,
        vk::ImageLayout::eShaderReadOnlyOptimal
    );

    deviceHandle.destroyBuffer(stagingBuffer);
    deviceHandle.freeMemory(stagingBufferMemory);
    SDL_DestroySurface(image);
    image = nullptr;
    // #endregion TEXTURE
}

std::unique_ptr<CVulkanRenderer> CVulkanRenderer::TryToCreate(const IVulkanWindow* const window) {
    try {
        return std::make_unique<CVulkanRenderer>(window);
    } catch (const std::exception& e) {
        Log::Error("Cannot initialize vulkan renderer: {}", e.what());
        return nullptr;
    }
}

void CVulkanRenderer::Draw(glm::mat4 view) {
    auto deviceHandle = m_context->GetDevice()->GetHandle();
    auto swapchainHandle = m_swapchain->GetHandle();

    UpdateUniformBuffer(m_swapchain->GetInfo().m_extent, m_uniformBuffersMapped, m_frameIndex, view);

    // #region ACQUIRE_IMAGE
    std::ignore = deviceHandle.waitForFences(m_inFlightFences[m_frameIndex], vk::True, std::numeric_limits<unsigned int>::max());

    std::uint32_t imageIndex;
    auto res = deviceHandle.acquireNextImageKHR(
        swapchainHandle,
        std::numeric_limits<unsigned int>::max(),
        m_currentImageAvailableSemaphores[m_frameIndex],
        VK_NULL_HANDLE,
        &imageIndex
    );
    if (res == vk::Result::eErrorOutOfDateKHR) {
        Resize(deviceHandle, m_swapchain.get(), m_frameBuffers, m_renderPass);
        return;
    }

    deviceHandle.resetFences(m_inFlightFences[m_frameIndex]);
    if (res != vk::Result::eSuccess && res != vk::Result::eSuboptimalKHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }
    // #endregion ACQUIRE_IMAGE

    // #region COMMAND_RECORD
    m_commandBuffers[m_frameIndex].reset();
    vk::CommandBufferBeginInfo beginInfo {};
    m_commandBuffers[m_frameIndex].begin(beginInfo);

    // #region RENDER_PASS_BEGIN
    vk::RenderPassBeginInfo renderPassInfo {};
    renderPassInfo.renderPass = m_renderPass;
    renderPassInfo.framebuffer = m_frameBuffers[imageIndex];
    renderPassInfo.renderArea.offset = { { 0, 0 } };
    renderPassInfo.renderArea.extent = m_swapchain->GetInfo().m_extent;

    vk::ClearValue clearColor = vk::ClearColorValue { 0.0f, 0.0f, 0.005f, 1.0f };
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    m_commandBuffers[m_frameIndex].beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
    // #endregion RENDER_PASS_BEGIN

    m_commandBuffers[m_frameIndex].bindPipeline(vk::PipelineBindPoint::eGraphics, m_pipeline);

    vk::Viewport viewport {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(m_swapchain->GetInfo().m_extent.width);
    viewport.height = static_cast<float>(m_swapchain->GetInfo().m_extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    m_commandBuffers[m_frameIndex].setViewport(0, 1, &viewport);

    vk::Rect2D scissor {};
    scissor.offset = { { 0, 0 } };
    scissor.extent = m_swapchain->GetInfo().m_extent;
    m_commandBuffers[m_frameIndex].setScissor(0, 1, &scissor);

    vk::Buffer vertexBuffers[] = { m_vertexBuffer };
    vk::DeviceSize offsets[] = { 0 };
    m_commandBuffers[m_frameIndex].bindVertexBuffers(0, vertexBuffers, offsets);
    m_commandBuffers[m_frameIndex].bindIndexBuffer(m_indexBuffer, 0, vk::IndexType::eUint16);

    m_commandBuffers[m_frameIndex].bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics,
        m_pipelineLayout,
        0,
        m_descriptorSets[m_frameIndex],
        {}
    );

    m_commandBuffers[m_frameIndex].drawIndexed(static_cast<std::uint32_t>(indices.size()), 1, 0, 0, 0);

    m_commandBuffers[m_frameIndex].endRenderPass();
    m_commandBuffers[m_frameIndex].end();
    // #endregion COMMAND_RECORD

    // #region COMMAND_SUBMIT
    vk::SubmitInfo submitInfo {};

    vk::Semaphore waitSemaphores[] = { m_currentImageAvailableSemaphores[m_frameIndex] };
    vk::PipelineStageFlags waitStages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_commandBuffers[m_frameIndex];

    vk::Semaphore signalSemaphores[] = { m_renderFinishedSemaphores[imageIndex] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    std::ignore = m_context->GetDevice()->GetGraphicsQueue().m_handle.submit(1, &submitInfo, m_inFlightFences[m_frameIndex]);
    // #endregion COMMAND_RECORD

    // #region PRESENT
    vk::PresentInfoKHR presentInfo {};

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchainHandle;

    presentInfo.pImageIndices = &imageIndex;

    res = m_context->GetDevice()->GetPresentQueue().m_handle.presentKHR(&presentInfo);
    if (res == vk::Result::eErrorOutOfDateKHR) {
        Resize(deviceHandle, m_swapchain.get(), m_frameBuffers, m_renderPass);
        return;
    }
    // #endregion PRESENT

    m_frameIndex = (m_frameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
}

CVulkanRenderer::~CVulkanRenderer() {
    vk::Device deviceHandle = m_context->GetDevice()->GetHandle();
    deviceHandle.waitIdle();
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        deviceHandle.destroyBuffer(m_uniformBuffers[i]);
        deviceHandle.freeMemory(m_uniformBuffersMemory[i]);
    }
    deviceHandle.destroyImage(m_texture);
    deviceHandle.freeMemory(m_textureMemory);
    deviceHandle.destroyDescriptorSetLayout(m_descriptorSetLayout);
    deviceHandle.destroyDescriptorPool(m_descriptorPool);
    deviceHandle.freeMemory(m_indexBufferMemory);
    deviceHandle.destroyBuffer(m_indexBuffer);
    deviceHandle.freeMemory(m_vertexBufferMemory);
    deviceHandle.destroyBuffer(m_vertexBuffer);
    for (auto ias : m_currentImageAvailableSemaphores) {
        deviceHandle.destroySemaphore(ias);
    }
    for (auto iff : m_inFlightFences) {
        deviceHandle.destroyFence(iff);
    }
    for (auto rfs : m_renderFinishedSemaphores) {
        deviceHandle.destroySemaphore(rfs);
    }
    deviceHandle.destroyCommandPool(m_commandPool);
    for (auto framebuffer : m_frameBuffers) {
        deviceHandle.destroyFramebuffer(framebuffer);
    }
    deviceHandle.destroyPipeline(m_pipeline);
    deviceHandle.destroyPipelineLayout(m_pipelineLayout);
    deviceHandle.destroyRenderPass(m_renderPass);
}
