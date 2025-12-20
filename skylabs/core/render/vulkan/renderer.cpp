#include <skylabs/core/render/vulkan/renderer.hpp>

#include <skylabs/public/logging.hpp>
#include <skylabs/core/render/vulkan/shader.hpp>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <SDL3_image/SDL_image.h>
#include <tiny_obj_loader.h>

#include <chrono>


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
    bool operator==(const Vertex& other) const {
        return pos == other.pos && color == other.color && texCoord == other.texCoord;
    }
};

namespace std {
    template<> struct hash<Vertex> {
        size_t operator()(Vertex const& vertex) const {
            return ((hash<glm::vec3>()(vertex.pos) ^
                   (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
                   (hash<glm::vec2>()(vertex.texCoord) << 1);
        }
    };
}

std::vector vertices {
    Vertex { .pos = { -0.5f, -0.5f, 0.0f }, .color = { 1.0f, 0.0f, 0.0f }, .texCoord = { 1.0f, 0.0f } },
    Vertex { .pos = {  0.5f, -0.5f, 0.0f }, .color = { 0.0f, 1.0f, 0.0f }, .texCoord = { 0.0f, 0.0f } },
    Vertex { .pos = {  0.5f,  0.5f, 0.0f }, .color = { 0.0f, 0.0f, 1.0f }, .texCoord = { 0.0f, 1.0f } },
    Vertex { .pos = { -0.5f,  0.5f, 0.0f }, .color = { 1.0f, 1.0f, 1.0f }, .texCoord = { 1.0f, 1.0f } },

   // Vertex { .pos = { -0.5f, -0.5f, 0.5f }, .color = { 1.0f, 1.0f, 0.0f }, .texCoord = { 0.0f, 0.0f } },
   // Vertex { .pos = {  0.5f, -0.5f, 0.5f }, .color = { 0.0f, 1.0f, 1.0f }, .texCoord = { 0.0f, 1.0f } },
   // Vertex { .pos = {  0.5f,  0.5f, 0.5f }, .color = { 1.0f, 0.0f, 1.0f }, .texCoord = { 1.0f, 1.0f } },
   // Vertex { .pos = { -0.5f,  0.5f, 0.5f }, .color = { 0.5f, 0.5f, 0.5f }, .texCoord = { 1.0f, 0.0f } },
};

struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
    glm::vec3 offset;
};

glm::vec3 offset = {0.0f, 0.0f, 0.0f}; // то что реально уходит в UBO
glm::vec3 targetOffset  = {0.0f, 0.0f, 0.0f}; // то куда хотим прийти

constexpr float lerpSpeed = 5.0f; // чем больше, тем быстрее двигается

void MoveForward()  { targetOffset.z += 0.1f; }
void MoveBackward() { targetOffset.z -= 0.1f; }

std::vector<std::uint16_t> indices = {
    0, 1, 2, 2, 3, 0,   // первый квадрат
    //4, 5, 6, 6, 7, 4    // второй квадрат
};

namespace {
std::pair<vk::Result, uint32_t> SwapchainNextImageWrapper(
    const vk::raii::SwapchainKHR& swapchain,
    uint64_t timeout,
    vk::Semaphore semaphore,
    vk::Fence fence
) {
    uint32_t image_index;
    vk::Result result = static_cast<vk::Result>(swapchain.getDispatcher()->vkAcquireNextImageKHR(
        static_cast<VkDevice>(swapchain.getDevice()), static_cast<VkSwapchainKHR>(*swapchain),
        timeout, static_cast<VkSemaphore>(semaphore), static_cast<VkFence>(fence), &image_index));
    return std::make_pair(result, image_index);
}

vk::Result QueuePresentWrapper(
    const vk::raii::Queue& queue,
    const vk::PresentInfoKHR& present_info
) {
    return static_cast<vk::Result>(queue.getDispatcher()->vkQueuePresentKHR(
        static_cast<VkQueue>(*queue), reinterpret_cast<const VkPresentInfoKHR*>(&present_info)));
}

auto CreateFrameBuffers(
    const vk::raii::Device& device,
    const Vulkan::CSwapchain& swapchain,
    const vk::raii::RenderPass& renderPass
) -> std::vector<vk::raii::Framebuffer> {
    std::vector<vk::raii::Framebuffer> out;

    out.reserve(swapchain.GetImageCount());
    for (const auto& imageView : swapchain.GetImageViews()) {
        std::array attachments = {
            *imageView
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
    Vulkan::CSwapchain& swapchain,
    std::vector<vk::raii::Framebuffer>& frameBuffers
) -> void {
    swapchain.Recreate();
    frameBuffers = CreateFrameBuffers(device, swapchain, renderPass);
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
    const vk::Extent2D& cameraDimensions,
    std::vector<Vulkan::CMemoryMapping>& uniformBuffersMapped,
    std::uint32_t currentImage,
    glm::mat4 view,
    float deltaTime
) {
    using namespace std::chrono;

    static auto startTime = high_resolution_clock::now();

    auto currentTime = high_resolution_clock::now();
    const float time = duration<float, seconds::period>(currentTime - startTime).count();

    UniformBufferObject ubo {};
    ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(5.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.view = view;
    ubo.proj = glm::perspective(glm::radians(90.0f), (float) cameraDimensions.width / (float) cameraDimensions.height, 0.01f, 10.0f);
    ubo.proj[1][1] *= -1;
    offset = glm::mix(offset, targetOffset, lerpSpeed * deltaTime);
    if (glm::length(targetOffset - offset) < 0.0001) {
        offset = targetOffset;
    }
    ubo.offset = offset;

    std::memcpy(*uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
}

std::uint32_t renderWidth = 0;
std::uint32_t renderHeight = 0;
}

namespace Vulkan {
CRenderer::CRenderer(const IWindow* const window) {
    if (window == nullptr) {
        throw std::runtime_error("Cannot initialize vulkan renderer. Window is nullptr");
    }
    m_context = CContext { window };

    m_surface = CSurface { m_context };

    m_swapchain = CSwapchain { m_context, *m_surface, 3, vk::PresentModeKHR::eImmediate };

    renderWidth = m_swapchain.GetExtent().width;
    renderHeight = m_swapchain.GetExtent().height;

    m_frameData.reserve(FRAMES_IN_FLIGHT_COUNT);
    for (std::size_t i = 0; i < FRAMES_IN_FLIGHT_COUNT; ++i) {
        m_frameData.emplace_back(m_context);
    }

    const std::uint32_t imageCount = m_swapchain.GetImageCount();
    m_renderFinishedSemaphores.reserve(imageCount);
    for (std::size_t i = 0; i < imageCount; ++i) {
        m_renderFinishedSemaphores.emplace_back(*m_context.GetDevice(), vk::SemaphoreCreateInfo {});
    }

    auto findSupportedFormat = [this](const std::vector<vk::Format>& candidates, const vk::ImageTiling tiling, const vk::FormatFeatureFlags& features) -> vk::Format {
        for (const vk::Format format : candidates) {
            const vk::FormatProperties props = m_context.GetPhysicalDevice()->GetHandle().getFormatProperties(format);

            if (tiling == vk::ImageTiling::eLinear && (props.linearTilingFeatures & features) == features) {
                return format;
            }
            if (tiling == vk::ImageTiling::eOptimal && (props.optimalTilingFeatures & features) == features) {
                return format;
            }
        }

        throw std::runtime_error("Failed to find supported format!");
    };

    auto findDepthFormat = [&] -> vk::Format {
        return findSupportedFormat(
            { vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint },
            vk::ImageTiling::eOptimal,
            vk::FormatFeatureFlagBits::eDepthStencilAttachment
        );
    };

    m_depthBuffer = CImage {
        m_context,
        vk::Extent3D { renderWidth, renderHeight, 1 },
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


    m_colorBuffer = CImage {
        m_context,
        vk::Extent3D { renderWidth, renderHeight, 1 },
        vk::Format::eR8G8B8A8Snorm,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
        vk::ImageAspectFlagBits::eColor,
        vk::MemoryPropertyFlagBits::eDeviceLocal
    };

    vk::AttachmentDescription colorAttachment {};
    colorAttachment.format = vk::Format::eR8G8B8A8Snorm;
    colorAttachment.samples = vk::SampleCountFlagBits::e1;
    colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
    colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
    colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    colorAttachment.initialLayout = vk::ImageLayout::eUndefined;
    colorAttachment.finalLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

    vk::AttachmentReference colorAttachmentRef {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;


    vk::SubpassDescription subpass {};
    subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
    subpass.inputAttachmentCount = 0;
    subpass.pInputAttachments = nullptr;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.preserveAttachmentCount = 0;
    subpass.pPreserveAttachments = nullptr;
    subpass.pResolveAttachments = nullptr;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    vk::SubpassDependency dependency {};
    dependency.srcSubpass = vk::SubpassExternal;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dependency.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
    dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;

    std::array attachments = { colorAttachment, depthAttachment };
    vk::RenderPassCreateInfo renderPassInfo {};
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;
    m_renderPassMain = m_context.GetDevice().GetHandle().createRenderPass(renderPassInfo);


    const CShader vertexShader(m_context, CShader::Type::eVertex, "shader.vert.spv");
    const CShader fragmentShader(m_context, CShader::Type::eFragment, "shader.frag.spv");

    const std::array shaderStages = {
        vertexShader.GetPipelineShaderCreateInfo(),
        fragmentShader.GetPipelineShaderCreateInfo(),
    };

    SDL_Surface* imageRaw = IMG_Load("assets/viking_room.png");
    if (!imageRaw) {
        throw std::runtime_error("Failed to load texture image!");
    }
    SDL_Surface* image = SDL_ConvertSurface(imageRaw, SDL_PIXELFORMAT_ABGR8888);
    SDL_DestroySurface(imageRaw);
    vk::DeviceSize imageSize = static_cast<vk::DeviceSize>(image->w) * image->h * 4;

    CHostBuffer stagingBuffer = CHostBuffer { m_context, imageSize, vk::BufferUsageFlagBits::eTransferSrc };
    auto commandPool = (*m_context.GetDevice()).createCommandPool({
            vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
            m_context.GetDevice().GetGraphicsQueue().m_familyIndex
    });
    {
        CMemoryMapping mapping = stagingBuffer.Map();
        std::memcpy(mapping.GetData(), image->pixels, imageSize);

        void* p = std::malloc(imageSize / 4);
        std::memcpy((char*)mapping.GetData() + imageSize * 3 / 4, p, imageSize / 4);
        std::free(p);
    }

    m_modelTexture = CImage { m_context,
                              { static_cast<uint32_t>(image->w), static_cast<uint32_t>(image->h), 1 },
                              vk::Format::eR8G8B8A8Srgb,
                              vk::ImageTiling::eOptimal,
                              vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
                              vk::ImageAspectFlagBits::eColor,
                              vk::MemoryPropertyFlagBits::eDeviceLocal };

    vk::raii::CommandBuffer commandBuffer = BeginSingleTimeCommands(*m_context.GetDevice(), commandPool);
    {
        m_modelTexture.TransitionLayout(commandBuffer, vk::ImageLayout::eTransferDstOptimal);
        m_modelTexture.CopyBufferToImage(commandBuffer, *stagingBuffer, { static_cast<uint32_t>(image->w), static_cast<uint32_t>(image->h), 1 });
        m_modelTexture.TransitionLayout(commandBuffer, vk::ImageLayout::eShaderReadOnlyOptimal);
    }
    EndSingleTimeCommands(m_context.GetDevice(), commandBuffer);

    {
        stagingBuffer.Clear();
    }
    SDL_DestroySurface(image);
    image = nullptr;

    m_modelTextureSampler = CSampler { m_context };


    const std::string MODEL_PATH = "assets/viking_room.obj";
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn;
    std::string err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, MODEL_PATH.c_str())) {
        throw std::runtime_error(warn + " " + err);
    }
    std::unordered_map<Vertex, uint32_t> uniqueVertices {};
    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            Vertex vertex {};

            vertex.pos = { attrib.vertices[3 * index.vertex_index + 0], attrib.vertices[3 * index.vertex_index + 1], attrib.vertices[3 * index.vertex_index + 2] };

            vertex.texCoord = { attrib.texcoords[2 * index.texcoord_index + 0], 1.0f - attrib.texcoords[2 * index.texcoord_index + 1] };

            vertex.color = { 1.0f, 1.0f, 1.0f };

            if (uniqueVertices.count(vertex) == 0) {
                uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                vertices.push_back(vertex);
            }

            indices.push_back(uniqueVertices[vertex]);
        }
    }





    vk::DeviceSize bufferSize = sizeof(UniformBufferObject);
    m_uniformBuffers.reserve(FRAMES_IN_FLIGHT_COUNT);
    m_uniformBuffersMapped.reserve(FRAMES_IN_FLIGHT_COUNT);
    for (size_t i = 0; i < FRAMES_IN_FLIGHT_COUNT; i++) {
        m_uniformBuffers.emplace_back(
            m_context,
            bufferSize,
            vk::BufferUsageFlagBits::eUniformBuffer
        );
        m_uniformBuffersMapped.emplace_back(m_uniformBuffers[i].Map());
    }

    vk::DescriptorSetLayoutBinding uboLayoutBinding {};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment;
    uboLayoutBinding.pImmutableSamplers = nullptr;

    vk::DescriptorSetLayoutBinding samplerLayoutBinding {};
    samplerLayoutBinding.binding = 1;
    samplerLayoutBinding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;
    samplerLayoutBinding.pImmutableSamplers = nullptr;

    std::array bindings = { uboLayoutBinding, samplerLayoutBinding };

    vk::DescriptorSetLayoutCreateInfo layoutInfo {};
    layoutInfo.bindingCount = static_cast<std::uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    m_descriptorSetLayoutMain = vk::raii::DescriptorSetLayout { m_context.GetDevice().GetHandle(), layoutInfo };


    std::array<vk::DescriptorPoolSize, 2> poolSizes {};
    poolSizes[0].type = vk::DescriptorType::eUniformBuffer;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(FRAMES_IN_FLIGHT_COUNT);
    poolSizes[1].type = vk::DescriptorType::eCombinedImageSampler;
    poolSizes[1].descriptorCount = static_cast<uint32_t>(FRAMES_IN_FLIGHT_COUNT);

    vk::DescriptorPoolCreateInfo poolInfo {};
    poolInfo.poolSizeCount = static_cast<std::uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(FRAMES_IN_FLIGHT_COUNT);

    m_descriptorPoolMain = vk::raii::DescriptorPool { m_context.GetDevice().GetHandle(), poolInfo };


    std::vector<vk::DescriptorSetLayout> layouts(FRAMES_IN_FLIGHT_COUNT, m_descriptorSetLayoutMain);
    vk::DescriptorSetAllocateInfo descriptorAllocInfo {};
    descriptorAllocInfo.descriptorPool = m_descriptorPoolMain;
    descriptorAllocInfo.descriptorSetCount = static_cast<uint32_t>(FRAMES_IN_FLIGHT_COUNT);
    descriptorAllocInfo.pSetLayouts = layouts.data();

    m_descriptorSetsMain = (**m_context.GetDevice()).allocateDescriptorSets(descriptorAllocInfo);

    for (size_t i = 0; i < FRAMES_IN_FLIGHT_COUNT; i++) {
        vk::DescriptorBufferInfo bufferInfo {};
        bufferInfo.buffer = *m_uniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        vk::DescriptorImageInfo imageInfo {};
        imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        imageInfo.imageView = m_modelTexture.GetView();
        imageInfo.sampler = *m_modelTextureSampler;

        std::array<vk::WriteDescriptorSet, 2> descriptorWrites{};
        descriptorWrites[0].dstSet = m_descriptorSetsMain[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = vk::DescriptorType::eUniformBuffer;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfo;
        descriptorWrites[0].pImageInfo = nullptr;
        descriptorWrites[0].pTexelBufferView = nullptr;

        descriptorWrites[1].dstSet = m_descriptorSetsMain[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = vk::DescriptorType::eCombinedImageSampler;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pImageInfo = &imageInfo;

        m_context.GetDevice().GetHandle().updateDescriptorSets(descriptorWrites, {});
    }



    m_pipelineMain = CPipeline {
        m_context,
        shaderStages,
        std::array { *m_descriptorSetLayoutMain },
        Vertex::GetBindingDescription(),
        Vertex::GetAttributeDescriptions(),
        m_renderPassMain
    };


    std::array attachmentsS = { *m_colorBuffer.GetView(), *m_depthBuffer.GetView() };
    vk::FramebufferCreateInfo framebufferInfo {};
    framebufferInfo.renderPass = m_renderPassMain;
    framebufferInfo.attachmentCount = static_cast<uint32_t>(attachmentsS.size());
    framebufferInfo.pAttachments = attachmentsS.data();
    framebufferInfo.width = renderWidth;
    framebufferInfo.height = renderHeight;
    framebufferInfo.layers = 1;
    m_frameBufferMain = vk::raii::Framebuffer { *m_context.GetDevice(), framebufferInfo };





    vk::DescriptorSetLayoutBinding samplerBinding {};
    samplerBinding.binding = 0;
    samplerBinding.descriptorCount = 1;
    samplerBinding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
    samplerBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;
    samplerBinding.pImmutableSamplers = nullptr;

    std::array bindingsSwapchain = { samplerBinding };

    layoutInfo = vk::DescriptorSetLayoutCreateInfo {};
    layoutInfo.bindingCount = static_cast<std::uint32_t>(bindingsSwapchain.size());
    layoutInfo.pBindings = bindingsSwapchain.data();

    m_descriptorSetLayoutSwapchain = vk::raii::DescriptorSetLayout { m_context.GetDevice().GetHandle(), layoutInfo };



    std::array<vk::DescriptorPoolSize, 1> poolSizesSwapchain {};
    poolSizesSwapchain[0].type = vk::DescriptorType::eCombinedImageSampler;
    poolSizesSwapchain[0].descriptorCount = static_cast<uint32_t>(FRAMES_IN_FLIGHT_COUNT);

    poolInfo = vk::DescriptorPoolCreateInfo {};
    poolInfo.poolSizeCount = static_cast<std::uint32_t>(poolSizesSwapchain.size());
    poolInfo.pPoolSizes = poolSizesSwapchain.data();
    poolInfo.maxSets = static_cast<uint32_t>(FRAMES_IN_FLIGHT_COUNT);

    m_descriptorPoolSwapchain = vk::raii::DescriptorPool { m_context.GetDevice().GetHandle(), poolInfo };



    layouts = std::vector<vk::DescriptorSetLayout>(FRAMES_IN_FLIGHT_COUNT, m_descriptorSetLayoutSwapchain);
    descriptorAllocInfo = vk::DescriptorSetAllocateInfo {};
    descriptorAllocInfo.descriptorPool = m_descriptorPoolSwapchain;
    descriptorAllocInfo.descriptorSetCount = static_cast<uint32_t>(FRAMES_IN_FLIGHT_COUNT);
    descriptorAllocInfo.pSetLayouts = layouts.data();

    m_descriptorSetsSwapchain = (**m_context.GetDevice()).allocateDescriptorSets(descriptorAllocInfo);

    m_mainSampler = CSampler {m_context};

    for (size_t i = 0; i < FRAMES_IN_FLIGHT_COUNT; i++) {
        vk::DescriptorImageInfo imageInfo {};
        imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        imageInfo.imageView = m_colorBuffer.GetView();
        imageInfo.sampler = *m_mainSampler;

        vk::WriteDescriptorSet descriptorWrite {};
        descriptorWrite.dstSet = m_descriptorSetsSwapchain[i];
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pImageInfo = &imageInfo;

        (*m_context.GetDevice()).updateDescriptorSets(descriptorWrite, nullptr);
    }



    
    colorAttachment = vk::AttachmentDescription {};
    colorAttachment.format = m_swapchain.GetSurfaceFormat().format;
    colorAttachment.samples = vk::SampleCountFlagBits::e1;
    colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
    colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
    colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    colorAttachment.initialLayout = vk::ImageLayout::eUndefined;
    colorAttachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;

    // Аттачмент референс, который описывает просто layout аттачмента
    colorAttachmentRef = vk::AttachmentReference {};
    colorAttachmentRef.attachment = 0; // Индекс в vk::SubpassDescription
    colorAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

    // Описание сабпасса
    subpass = vk::SubpassDescription {};
    subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
    subpass.inputAttachmentCount = 0;
    subpass.pInputAttachments = nullptr;
    // Аттачмент в который будем писать
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.preserveAttachmentCount = 0;
    subpass.pPreserveAttachments = nullptr;
    subpass.pResolveAttachments = nullptr;
    subpass.pDepthStencilAttachment = nullptr;

    // Описываем как сабпассы будут связаны
    dependency = vk::SubpassDependency {};
    dependency.srcSubpass = vk::SubpassExternal; // Пустой внешний сабпасс
    dependency.dstSubpass = 0; // Описание применяется к первому (нулевому) сабпассу
    dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput; // Ждём когда на этой стадии закончатся операции
    dependency.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite; // Какие конкретно операции. Пустое значит все операции
    dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput; // На какую стадию идём
    dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite; // Что делаем

    std::array attachmentsSwapchain = { colorAttachment };
    renderPassInfo = vk::RenderPassCreateInfo {};
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachmentsSwapchain.size());
    renderPassInfo.pAttachments = attachmentsSwapchain.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;
    m_renderPassSwapchain = m_context.GetDevice().GetHandle().createRenderPass(renderPassInfo);
    //endregion Subpasses



    //region PIPELINE
    const CShader vertexShaderSwapchain(m_context, CShader::Type::eVertex, "shaderSwapchain.vert.spv");
    const CShader fragmentShaderSwapchain(m_context, CShader::Type::eFragment, "shaderSwapchain.frag.spv");

    const std::array shaderStagesSwapchain = {
        vertexShaderSwapchain.GetPipelineShaderCreateInfo(),
        fragmentShaderSwapchain.GetPipelineShaderCreateInfo(),
    };

    m_pipelineSwapchain = CPipeline {
        m_context,
        shaderStagesSwapchain,
        std::array { *m_descriptorSetLayoutSwapchain },
        {}, {}, m_renderPassSwapchain
    };

    m_frameBuffersSwapchain = CreateFrameBuffers(m_context.GetDevice().GetHandle(), m_swapchain, m_renderPassSwapchain);

    //region VERTEX BUFFER
    vk::DeviceSize vertexBufferSize = sizeof(vertices[0]) * vertices.size();

    stagingBuffer = CHostBuffer {
        m_context,
        vertexBufferSize,
        vk::BufferUsageFlagBits::eTransferSrc
    };

    {
        CMemoryMapping mapping = stagingBuffer.Map();
        std::memcpy(mapping.GetData(), vertices.data(), vertexBufferSize);
    }

    m_vertexBuffer = CDeviceBuffer {
        m_context,
        vertexBufferSize,
        vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer
    };

    CopyBuffer(
        m_context.GetDevice(),
        commandPool,
        *stagingBuffer,
        *m_vertexBuffer,
        vertexBufferSize
    );

    {
        stagingBuffer.Clear();
    }
    //endregion VERTEX BUFFER

    //region INDEX BUFFER
    vk::DeviceSize indexBufferSize = sizeof(indices[0]) * indices.size();

    stagingBuffer = CHostBuffer {
        m_context,
        indexBufferSize,
        vk::BufferUsageFlagBits::eTransferSrc
    };

    {
        CMemoryMapping mapping = stagingBuffer.Map();
        std::memcpy(mapping.GetData(), indices.data(), indexBufferSize);
    }

    m_indexBuffer = CDeviceBuffer {
        m_context,
        indexBufferSize,
        vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer
    };

    CopyBuffer(
        m_context.GetDevice(),
        commandPool,
        *stagingBuffer,
        *m_indexBuffer,
        indexBufferSize
    );

    {
        stagingBuffer.Clear();
    }
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
    CFrameData& frameData = m_frameData[m_frameIndex];
    const vk::raii::Device& deviceHandle = m_context.GetDevice().GetHandle();
    const vk::raii::SwapchainKHR& swapchainHandle = m_swapchain.GetHandle();


    UpdateUniformBuffer(m_swapchain.GetExtent(), m_uniformBuffersMapped, m_frameIndex, view, deltaTime);

    // #region ACQUIRE_IMAGE
    vk::Result result = deviceHandle.waitForFences({ frameData.GetFence() }, vk::True, std::numeric_limits<std::uint64_t>::max());
    if (result != vk::Result::eSuccess) {
        throw std::runtime_error("Failed to wait for fence: " + vk::to_string(result));
    }

    std::uint32_t imageIndex;
    std::tie(result, imageIndex) = SwapchainNextImageWrapper(
        swapchainHandle,
        std::numeric_limits<std::uint64_t>::max(),
        *frameData.GetImageAvailableSemaphore(),
        nullptr
    );

    if (result == vk::Result::eErrorOutOfDateKHR) {
        frameData.RecreateImageAvailableSemaphore();
        for (auto& frame : m_frameData) {
            result = deviceHandle.waitForFences({ frame.GetFence() }, vk::True, std::numeric_limits<std::uint64_t>::max());
            if (result != vk::Result::eSuccess) {
                throw std::runtime_error("Failed to wait for fence: " + vk::to_string(result));
            }
        }
        Resize(m_context.GetDevice().GetHandle(), m_renderPassSwapchain, m_swapchain, m_frameBuffersSwapchain);

        return;
    }
    if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR) {
        throw std::runtime_error("Failed to acquire swapchain image: " + vk::to_string(result));
    }

    // Reset fence before we can return from function to avoid deadlock
    deviceHandle.resetFences({ frameData.GetFence() });
    // #endregion ACQUIRE_IMAGE


    // #region COMMAND_RECORD
    frameData.GetCommandBuffers()[0].reset();
    vk::CommandBufferBeginInfo beginInfo {};
    frameData.GetCommandBuffers()[0].begin(beginInfo);


    vk::RenderPassBeginInfo renderPassInfo {};
    renderPassInfo.renderPass = m_renderPassMain;
    renderPassInfo.framebuffer = m_frameBufferMain;
    renderPassInfo.renderArea.offset = { { 0, 0 } };
    renderPassInfo.renderArea.extent = vk::Extent2D { renderWidth, renderHeight };

    std::array<vk::ClearValue, 2> clearValuesMain {};
    clearValuesMain[0].color = vk::ClearColorValue { 0.2f, 0.3f, 0.6f, 1.0f };
    clearValuesMain[1].depthStencil = vk::ClearDepthStencilValue { 1.0f, 0 };

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValuesMain.size());
    renderPassInfo.pClearValues = clearValuesMain.data();
    frameData.GetCommandBuffers()[0].beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);

        frameData.GetCommandBuffers()[0].bindPipeline(vk::PipelineBindPoint::eGraphics, *m_pipelineMain);

    vk::Viewport viewport {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = renderWidth;
    viewport.height = renderHeight;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    frameData.GetCommandBuffers()[0].setViewport(0, viewport);

    vk::Rect2D scissor {};
    scissor.offset = { { 0, 0 } };
    scissor.extent = vk::Extent2D { renderWidth, renderHeight };
    frameData.GetCommandBuffers()[0].setScissor(0, scissor);

    vk::Buffer vertexBuffers[] = { *m_vertexBuffer };
    vk::DeviceSize offsets[] = { 0 };
    frameData.GetCommandBuffers()[0].bindVertexBuffers(0, vertexBuffers, offsets);
    frameData.GetCommandBuffers()[0].bindIndexBuffer(*m_indexBuffer, 0, vk::IndexType::eUint16);

    frameData.GetCommandBuffers()[0].bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *m_pipelineMain.GetLayout(), 0, m_descriptorSetsMain[m_frameIndex], {});

    frameData.GetCommandBuffers()[0].drawIndexed(indices.size(), 1, 0, 0, 0);


    frameData.GetCommandBuffers()[0].endRenderPass();


    // #region RENDER_PASS_BEGIN
    renderPassInfo = vk::RenderPassBeginInfo {};
    renderPassInfo.renderPass = m_renderPassSwapchain;
    renderPassInfo.framebuffer = m_frameBuffersSwapchain[imageIndex];
    renderPassInfo.renderArea.offset = { { 0, 0 } };
    renderPassInfo.renderArea.extent = m_swapchain.GetExtent();

    std::array<vk::ClearValue, 1> clearValues{};
    clearValues[0].color = vk::ClearColorValue { 0.0f, 0.0f, 0.0f, 1.0f };
    //clearValues[1].depthStencil = vk::ClearDepthStencilValue { 1.0f, 0 };

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    frameData.GetCommandBuffers()[0].beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
    // #endregion RENDER_PASS_BEGIN

    frameData.GetCommandBuffers()[0].bindPipeline(vk::PipelineBindPoint::eGraphics, *m_pipelineSwapchain);

     viewport = vk::Viewport {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(m_swapchain.GetExtent().width);
    viewport.height = static_cast<float>(m_swapchain.GetExtent().height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    frameData.GetCommandBuffers()[0].setViewport(0, viewport);

     scissor = vk::Rect2D {};
    scissor.offset = { { 0, 0 } };
    scissor.extent = m_swapchain.GetExtent();
    frameData.GetCommandBuffers()[0].setScissor(0, scissor);

    frameData.GetCommandBuffers()[0].bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics,
        *m_pipelineSwapchain.GetLayout(),
        0,
        m_descriptorSetsSwapchain[m_frameIndex],
        {}
    );

    frameData.GetCommandBuffers()[0].draw(3, 1, 0, 0);

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

    const CDevice& device = m_context.GetDevice();
    device.GetGraphicsQueue().m_handle.submit(submitInfo, frameData.GetFence());
    // #endregion COMMAND_RECORD

    // #region PRESENT
    vk::PresentInfoKHR presentInfo {};

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &*swapchainHandle;

    presentInfo.pImageIndices = &imageIndex;

    result = QueuePresentWrapper(device.GetPresentQueue().m_handle, presentInfo);
    if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR || GetResizedState()) {
        SetResizedState(false);
        frameData.RecreateImageAvailableSemaphore();
        for (auto& frame : m_frameData) {
            result = deviceHandle.waitForFences({ frame.GetFence() }, vk::True, std::numeric_limits<std::uint64_t>::max());
            if (result != vk::Result::eSuccess) {
                throw std::runtime_error("Failed to wait for fence: " + vk::to_string(result));
            }
        }
        Resize(m_context.GetDevice().GetHandle(), m_renderPassSwapchain, m_swapchain, m_frameBuffersSwapchain);

        return;
    }
    // #endregion PRESENT

    m_frameIndex = (m_frameIndex + 1) % FRAMES_IN_FLIGHT_COUNT;
}

CRenderer::~CRenderer() {
    m_context.GetDevice().GetHandle().waitIdle();
}
}
