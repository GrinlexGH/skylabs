#include <skylabs/core/render/vulkan/renderer.hpp>

#include <skylabs/public/logging.hpp>
#include <skylabs/core/render/vulkan/shader.hpp>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <SDL3_image/SDL_image.h>
#include <tiny_obj_loader.h>

#include <chrono>

template<> struct std::hash<CVertex> {
    size_t operator()(CVertex const& vertex) const noexcept {
        return ((hash<glm::vec3>()(vertex.m_position) ^
               (hash<glm::vec3>()(vertex.m_color) << 1)) >> 1) ^
               (hash<glm::vec2>()(vertex.m_texCoord) << 1);
    }
};

std::vector vertices {
    CVertex { .m_position = { -0.5f, -0.5f, 0.0f }, .m_color = { 1.0f, 0.0f, 0.0f }, .m_texCoord = { 1.0f, 0.0f } },
    CVertex { .m_position = {  0.5f, -0.5f, 0.0f }, .m_color = { 0.0f, 1.0f, 0.0f }, .m_texCoord = { 0.0f, 0.0f } },
    CVertex { .m_position = {  0.5f,  0.5f, 0.0f }, .m_color = { 0.0f, 0.0f, 1.0f }, .m_texCoord = { 0.0f, 1.0f } },
    CVertex { .m_position = { -0.5f,  0.5f, 0.0f }, .m_color = { 1.0f, 1.0f, 1.0f }, .m_texCoord = { 1.0f, 1.0f } },

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
    //4, 5, 6, 6, 7, 4 // второй квадрат
};

namespace {
std::pair<vk::Result, uint32_t> SwapchainNextImageWrapper(
    const vk::raii::SwapchainKHR& swapchain,
    const uint64_t timeout,
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
    const vk::RenderPass renderPass
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
    const vk::RenderPass renderPass,
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

    device.GraphicsQueue().m_handle.submit(submitInfo);
    device.GraphicsQueue().m_handle.waitIdle(); // TODO: USE FENCES

    commandBuffer.clear();
}

void generateMipmaps(
    const vk::raii::PhysicalDevice& physicalDevice,
    const vk::raii::CommandBuffer& cmd,
    Vulkan::CImage& image
) {
    const vk::FormatProperties formatProperties = physicalDevice.getFormatProperties(image.Format());

    if (!(formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear)) {
        throw std::runtime_error("texture image format does not support linear blitting!");
    }

    vk::ImageMemoryBarrier barrier{};
    barrier.image = *image;
    barrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
    barrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;
    barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;

    int32_t mipWidth = image.Extent().width;
    int32_t mipHeight = image.Extent().height;

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
            vk::Filter::eLinear
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
    vk::Buffer srcBuffer,
    vk::Buffer dstBuffer,
    vk::DeviceSize size
) {
    vk::raii::CommandBuffer commandBuffer = BeginSingleTimeCommands(device.Handle(), commandPool);

    vk::BufferCopy copyRegion;
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size = size;

    commandBuffer.copyBuffer(srcBuffer, dstBuffer, copyRegion);

    EndSingleTimeCommands(device, commandBuffer);
}

void UpdateUniformBuffer(
    const vk::Extent2D& cameraDimensions,
    const std::vector<Vulkan::CMemoryMapping>& uniformBuffersMapped,
    const std::uint32_t currentImage,
    const glm::mat4& view,
    const float deltaTime
) {
    using namespace std::chrono;

    static auto startTime = high_resolution_clock::now();

    const auto currentTime = high_resolution_clock::now();
    const float time = duration<float>(currentTime - startTime).count();

    UniformBufferObject ubo {};
    ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(5.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.view = view;
    ubo.proj = glm::perspective(glm::radians(90.0f), static_cast<float>(cameraDimensions.width) / static_cast<float>(cameraDimensions.height), 0.01f, 10.0f);
    ubo.proj[1][1] *= -1;
    offset = glm::mix(offset, targetOffset, lerpSpeed * deltaTime);
    if (glm::length(targetOffset - offset) < 0.0001) {
        offset = targetOffset;
    }
    ubo.offset = offset;

    std::memcpy(*uniformBuffersMapped.at(currentImage), &ubo, sizeof(ubo));
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
    m_swapchain = CSwapchain { m_context, *m_surface, 2, vk::PresentModeKHR::eImmediate };

    renderWidth = m_swapchain.GetExtent().width;
    renderHeight = m_swapchain.GetExtent().height;

    m_frameData.reserve(FRAMES_IN_FLIGHT_COUNT);
    for (std::size_t i = 0; i < FRAMES_IN_FLIGHT_COUNT; ++i) {
        m_frameData.emplace_back(m_context);
    }

    const std::uint32_t imageCount = m_swapchain.GetImageCount();
    m_renderFinishedSemaphores.reserve(imageCount);
    for (std::size_t i = 0; i < imageCount; ++i) {
        m_renderFinishedSemaphores.emplace_back(*m_context.Device(), vk::SemaphoreCreateInfo {});
    }

    auto findSupportedFormat = [this](const std::vector<vk::Format>& candidates, const vk::ImageTiling tiling, const vk::FormatFeatureFlags& features) -> vk::Format {
        for (const vk::Format format : candidates) {
            const vk::FormatProperties props = m_context.PhysicalDevice()->getFormatProperties(format);

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

    m_colorBuffer = CImage {
        m_context,
        vk::Extent3D { renderWidth, renderHeight, 1 },
        vk::Format::eR8G8B8A8Srgb,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
        vk::ImageAspectFlagBits::eColor,
        vk::MemoryPropertyFlagBits::eDeviceLocal
    };

    m_colorBufferMSAA = CImage {
        m_context,
        vk::Extent3D { renderWidth, renderHeight, 1 },
        vk::Format::eR8G8B8A8Srgb,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eColorAttachment,
        vk::ImageAspectFlagBits::eColor,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        1,
        vk::SampleCountFlagBits::e8
    };

    m_depthBufferMSAA = CImage {
        m_context,
        vk::Extent3D { renderWidth, renderHeight, 1 },
        findDepthFormat(),
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eDepthStencilAttachment,
        vk::ImageAspectFlagBits::eDepth,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        1,
        vk::SampleCountFlagBits::e8
    };

    vk::DebugUtilsObjectNameInfoEXT nameInfo;
    nameInfo.objectType = vk::ObjectType::eImage;
    nameInfo.objectHandle = reinterpret_cast<uint64_t>(static_cast<VkImage>(*m_depthBufferMSAA));
    nameInfo.pObjectName = "Main Depth Texture";
    m_context.Device()->setDebugUtilsObjectNameEXT(nameInfo);

    m_mainPass = CLegacyRenderPass {
        m_context,
        {
            .m_colorImages = {{{ .m_image = &m_colorBufferMSAA, .m_usage = ImageUsage::eMSAAWrite }}},
            .m_resolveImages = {{{ .m_image = &m_colorBuffer, .m_usage = ImageUsage::eShaderRead }}},
            .m_depthImage = &m_depthBufferMSAA
        }
    };

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
    const vk::DeviceSize imageSize = static_cast<vk::DeviceSize>(image->w) * image->h * 4;

    auto stagingBuffer = CHostBuffer { m_context, imageSize, vk::BufferUsageFlagBits::eTransferSrc };
    auto commandPool = m_context.Device()->createCommandPool({
            vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
            m_context.Device().GraphicsQueue().m_familyIndex
    });
    {
        const CMemoryMapping mapping = stagingBuffer.Map();
        std::memcpy(mapping.Data(), image->pixels, imageSize);
    }

    m_modelTexture = CImage {
        m_context,
        { static_cast<uint32_t>(image->w), static_cast<uint32_t>(image->h), 1 },
        vk::Format::eR8G8B8A8Srgb,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
        vk::ImageAspectFlagBits::eColor,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        static_cast<uint32_t>(std::floor(std::log2(std::max(image->w, image->h)))) + 1
    };

    vk::raii::CommandBuffer commandBuffer = BeginSingleTimeCommands(*m_context.Device(), commandPool);
    {
        m_modelTexture.TransitionLayout(commandBuffer, vk::ImageLayout::eTransferDstOptimal);
        m_modelTexture.CopyBufferToImage(commandBuffer, *stagingBuffer, { static_cast<uint32_t>(image->w), static_cast<uint32_t>(image->h), 1 });
        generateMipmaps(*m_context.PhysicalDevice(), commandBuffer, m_modelTexture);
    }
    EndSingleTimeCommands(m_context.Device(), commandBuffer);

    SDL_DestroySurface(image);

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
    std::unordered_map<CVertex, uint32_t> uniqueVertices {};
    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            CVertex vertex {};
            vertex.m_position = { attrib.vertices[3 * index.vertex_index + 0], attrib.vertices[3 * index.vertex_index + 1], attrib.vertices[3 * index.vertex_index + 2] };
            vertex.m_texCoord = { attrib.texcoords[2 * index.texcoord_index + 0], 1.0f - attrib.texcoords[2 * index.texcoord_index + 1] };
            vertex.m_color = { 1.0f, 1.0f, 1.0f };

            if (!uniqueVertices.contains(vertex)) {
                uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                vertices.push_back(vertex);
            }

            indices.push_back(uniqueVertices[vertex]);
        }
    }

    const vk::DeviceSize vertexBufferSize = sizeof(vertices[0]) * vertices.size();
    if (stagingBuffer.Size() < vertexBufferSize) {
        stagingBuffer = CHostBuffer {
            m_context,
            vertexBufferSize,
            vk::BufferUsageFlagBits::eTransferSrc
        };
    }

    {
        const CMemoryMapping mapping = stagingBuffer.Map();
        std::memcpy(mapping.Data(), vertices.data(), vertexBufferSize);
    }

    m_vertexBuffer = CDeviceBuffer {
        m_context,
        vertexBufferSize,
        vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer
    };

    CopyBuffer(
        m_context.Device(),
        commandPool,
        *stagingBuffer,
        *m_vertexBuffer,
        vertexBufferSize
    );

    const vk::DeviceSize indexBufferSize = sizeof(indices[0]) * indices.size();
    if (stagingBuffer.Size() < indexBufferSize) {
        stagingBuffer = CHostBuffer {
            m_context,
            indexBufferSize,
            vk::BufferUsageFlagBits::eTransferSrc
        };
    }

    {
        CMemoryMapping mapping = stagingBuffer.Map();
        std::memcpy(mapping.Data(), indices.data(), indexBufferSize);
    }

    m_indexBuffer = CDeviceBuffer {
        m_context,
        indexBufferSize,
        vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer
    };

    CopyBuffer(
        m_context.Device(),
        commandPool,
        *stagingBuffer,
        *m_indexBuffer,
        indexBufferSize
    );

    m_uniformBuffers.reserve(FRAMES_IN_FLIGHT_COUNT);
    m_uniformBuffersMapped.reserve(FRAMES_IN_FLIGHT_COUNT);
    for (size_t i = 0; i < FRAMES_IN_FLIGHT_COUNT; i++) {
        m_uniformBuffers.emplace_back(
            m_context,
            sizeof(UniformBufferObject),
            vk::BufferUsageFlagBits::eUniformBuffer
        );
        m_uniformBuffersMapped.emplace_back(m_uniformBuffers.at(i).Map());
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

    std::array bindings = { uboLayoutBinding, samplerLayoutBinding }; //!!!!

    vk::DescriptorSetLayoutCreateInfo layoutInfo {};
    layoutInfo.bindingCount = static_cast<std::uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    m_descriptorSetLayoutMain = vk::raii::DescriptorSetLayout { m_context.Device().Handle(), layoutInfo };


    std::array<vk::DescriptorPoolSize, 2> poolSizes {};
    poolSizes[0].type = vk::DescriptorType::eUniformBuffer;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(FRAMES_IN_FLIGHT_COUNT);
    poolSizes[1].type = vk::DescriptorType::eCombinedImageSampler;
    poolSizes[1].descriptorCount = static_cast<uint32_t>(FRAMES_IN_FLIGHT_COUNT);

    vk::DescriptorPoolCreateInfo poolInfo {};
    poolInfo.poolSizeCount = static_cast<std::uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(FRAMES_IN_FLIGHT_COUNT);

    m_descriptorPoolMain = vk::raii::DescriptorPool { m_context.Device().Handle(), poolInfo };


    std::vector<vk::DescriptorSetLayout> layouts(FRAMES_IN_FLIGHT_COUNT, m_descriptorSetLayoutMain);
    vk::DescriptorSetAllocateInfo descriptorAllocInfo {};
    descriptorAllocInfo.descriptorPool = m_descriptorPoolMain;
    descriptorAllocInfo.descriptorSetCount = static_cast<uint32_t>(FRAMES_IN_FLIGHT_COUNT);
    descriptorAllocInfo.pSetLayouts = layouts.data();

    m_descriptorSetsMain = (**m_context.Device()).allocateDescriptorSets(descriptorAllocInfo);

    for (size_t i = 0; i < FRAMES_IN_FLIGHT_COUNT; i++) {
        vk::DescriptorBufferInfo bufferInfo {};
        bufferInfo.buffer = *m_uniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        vk::DescriptorImageInfo imageInfo {};
        imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        imageInfo.imageView = m_modelTexture.View();
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

        m_context.Device()->updateDescriptorSets(descriptorWrites, {});
    }



    m_pipelineMain = CPipeline {
        m_context,
        shaderStages,
        std::array { *m_descriptorSetLayoutMain },
        CVertexFormat { CVertex::GetAttributes() },
        vk::SampleCountFlagBits::e8,
        m_mainPass.RenderPass()
    };


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

    m_descriptorSetLayoutSwapchain = vk::raii::DescriptorSetLayout { m_context.Device().Handle(), layoutInfo };



    std::array<vk::DescriptorPoolSize, 1> poolSizesSwapchain {};
    poolSizesSwapchain[0].type = vk::DescriptorType::eCombinedImageSampler;
    poolSizesSwapchain[0].descriptorCount = static_cast<uint32_t>(FRAMES_IN_FLIGHT_COUNT);

    poolInfo = vk::DescriptorPoolCreateInfo {};
    poolInfo.poolSizeCount = static_cast<std::uint32_t>(poolSizesSwapchain.size());
    poolInfo.pPoolSizes = poolSizesSwapchain.data();
    poolInfo.maxSets = static_cast<uint32_t>(FRAMES_IN_FLIGHT_COUNT);

    m_descriptorPoolSwapchain = vk::raii::DescriptorPool { m_context.Device().Handle(), poolInfo };



    layouts = std::vector<vk::DescriptorSetLayout>(FRAMES_IN_FLIGHT_COUNT, m_descriptorSetLayoutSwapchain);
    descriptorAllocInfo = vk::DescriptorSetAllocateInfo {};
    descriptorAllocInfo.descriptorPool = m_descriptorPoolSwapchain;
    descriptorAllocInfo.descriptorSetCount = static_cast<uint32_t>(FRAMES_IN_FLIGHT_COUNT);
    descriptorAllocInfo.pSetLayouts = layouts.data();

    m_descriptorSetsSwapchain = (**m_context.Device()).allocateDescriptorSets(descriptorAllocInfo);

    m_mainSampler = CSampler { m_context };

    for (size_t i = 0; i < FRAMES_IN_FLIGHT_COUNT; i++) {
        vk::DescriptorImageInfo imageInfo {};
        imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        imageInfo.imageView = m_colorBuffer.View();
        imageInfo.sampler = *m_mainSampler;

        vk::WriteDescriptorSet descriptorWrite {};
        descriptorWrite.dstSet = m_descriptorSetsSwapchain[i];
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pImageInfo = &imageInfo;

        m_context.Device()->updateDescriptorSets(descriptorWrite, nullptr);
    }

    m_swapchainPass = CLegacyRenderPass {
        m_context,
        {
            .m_colorImages = {
                {
                    { .m_image = &m_colorBuffer, .m_usage = ImageUsage::eSwapchainPresent },
                }}
        }
    };

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
        CVertexFormat {{}},
        vk::SampleCountFlagBits::e1,
        m_swapchainPass.RenderPass()
    };

    m_frameBuffersSwapchain = CreateFrameBuffers(m_context.Device().Handle(), m_swapchain, m_swapchainPass.RenderPass());
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
    const vk::raii::Device& deviceHandle = m_context.Device().Handle();
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
        for (auto& frame : m_frameData) {
            result = deviceHandle.waitForFences({ frame.GetFence() }, vk::True, std::numeric_limits<std::uint64_t>::max());
            if (result != vk::Result::eSuccess) {
                throw std::runtime_error("Failed to wait for fence: " + vk::to_string(result));
            }
        }
        frameData.RecreateImageAvailableSemaphore();
        Resize(m_context.Device().Handle(), m_swapchainPass.RenderPass(), m_swapchain, m_frameBuffersSwapchain);

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

    vk::DebugUtilsLabelEXT marker;
    marker.pLabelName = "Point of Interest";
    marker.color = std::array { 1.0f, 0.5f, 0.0f, 1.0f };
    frameData.GetCommandBuffers()[0].insertDebugUtilsLabelEXT(marker);

    vk::RenderPassBeginInfo renderPassInfo {};
    renderPassInfo.renderPass = m_mainPass.RenderPass();
    renderPassInfo.framebuffer = m_mainPass.Framebuffer();
    renderPassInfo.renderArea.offset = { { 0, 0 } };
    renderPassInfo.renderArea.extent = vk::Extent2D { renderWidth, renderHeight };

    std::array<vk::ClearValue, 3> clearValuesMain {};
    clearValuesMain[0].color = vk::ClearColorValue { 0.0f, 0.0f, 0.0f, 1.0f };
    clearValuesMain[1].color = vk::ClearColorValue { 1.0f, 0.3f, 0.6f, 1.0f };
    clearValuesMain[2].depthStencil = vk::ClearDepthStencilValue { 1.0f, 0 };

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

    frameData.GetCommandBuffers()[0].bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *m_pipelineMain.Layout(), 0, m_descriptorSetsMain[m_frameIndex], {});

    frameData.GetCommandBuffers()[0].drawIndexed(indices.size(), 1, 0, 0, 0);


    frameData.GetCommandBuffers()[0].endRenderPass();


    // #region RENDER_PASS_BEGIN
    vk::DebugUtilsLabelEXT labelInfo;
    labelInfo.pLabelName = "Latest";
    labelInfo.color = std::array { 1.0f, 0.5f, 0.0f, 1.0f };
    frameData.GetCommandBuffers()[0].beginDebugUtilsLabelEXT(labelInfo);

    renderPassInfo = vk::RenderPassBeginInfo {};
    renderPassInfo.renderPass = m_swapchainPass.RenderPass();
    renderPassInfo.framebuffer = m_frameBuffersSwapchain[imageIndex];
    renderPassInfo.renderArea.offset = { { .x = 0, .y = 0 } };
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
        *m_pipelineSwapchain.Layout(),
        0,
        m_descriptorSetsSwapchain[m_frameIndex],
        {}
    );

    frameData.GetCommandBuffers()[0].draw(3, 1, 0, 0);

    frameData.GetCommandBuffers()[0].endRenderPass();
    frameData.GetCommandBuffers()[0].endDebugUtilsLabelEXT();
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

    const CDevice& device = m_context.Device();
    device.GraphicsQueue().m_handle.submit(submitInfo, frameData.GetFence());
    // #endregion COMMAND_RECORD

    // #region PRESENT
    vk::PresentInfoKHR presentInfo {};

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &*swapchainHandle;

    presentInfo.pImageIndices = &imageIndex;

    result = QueuePresentWrapper(device.PresentQueue().m_handle, presentInfo);
    if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR || GetResizedState()) {
        SetResizedState(false);
        for (auto& frame : m_frameData) {
            result = deviceHandle.waitForFences({ frame.GetFence() }, vk::True, std::numeric_limits<std::uint64_t>::max());
            if (result != vk::Result::eSuccess) {
                throw std::runtime_error("Failed to wait for fence: " + vk::to_string(result));
            }
        }
        frameData.RecreateImageAvailableSemaphore();
        Resize(m_context.Device().Handle(), m_swapchainPass.RenderPass(), m_swapchain, m_frameBuffersSwapchain);

        return;
    }
    // #endregion PRESENT

    m_frameIndex = (m_frameIndex + 1) % FRAMES_IN_FLIGHT_COUNT;
}

CRenderer::~CRenderer() {
    m_context.Device().Handle().waitIdle();
}
}
