#pragma once

#include "renderer.hpp"
#include "vulkan_renderer_context.hpp"

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>

#include <glm/glm.hpp>
#include <vk_mem_alloc.hpp>

#include <unordered_map>
#include <optional>

struct CVertex {
    glm::vec2 pos;
    glm::vec3 color;
    glm::vec2 texCoord;

    static vk::VertexInputBindingDescription getBindingDescription() {
        vk::VertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(CVertex);
        bindingDescription.inputRate = vk::VertexInputRate::eVertex;

        return bindingDescription;
    }

    static std::array<vk::VertexInputAttributeDescription, 3> getAttributeDescriptions() {
        std::array<vk::VertexInputAttributeDescription, 3> attributeDescriptions{};

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = vk::Format::eR32G32Sfloat;
        attributeDescriptions[0].offset = offsetof(CVertex, pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = vk::Format::eR32G32Sfloat;
        attributeDescriptions[1].offset = offsetof(CVertex, color);

        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = vk::Format::eR32G32Sfloat;
        attributeDescriptions[2].offset = offsetof(CVertex, texCoord);

        return attributeDescriptions;
    }
};

class CVulkanRenderer final : public IRenderer {
public:
    CVulkanRenderer() = delete;
    explicit CVulkanRenderer(IWindow* window);
    CVulkanRenderer(const CVulkanRenderer&) = default;
    CVulkanRenderer(CVulkanRenderer&&) = default;
    CVulkanRenderer& operator=(const CVulkanRenderer&) = default;
    CVulkanRenderer& operator=(CVulkanRenderer&&) = default;
    ~CVulkanRenderer();

    void Draw() override;

    bool m_frameBufferResized = false;

private:
    void RecreateSwapChain();
    void CleanupSwapChain();
    void UpdateUniformBuffer(uint32_t currentImage, vk::Extent2D swapChainExtent);

    struct CQueueFamilyIndices
    {
        std::optional<uint32_t> m_graphicsFamily;
        std::optional<uint32_t> m_presentFamily;

        bool isComplete() const {
            return m_graphicsFamily.has_value() && m_presentFamily.has_value();
        }
    };

    struct CSwapChainInfo
    {
        vk::SurfaceCapabilitiesKHR m_capabilities;
        std::vector<vk::SurfaceFormatKHR> m_formats;
        std::vector<vk::PresentModeKHR> m_presentModes;
    };

    struct CBuffer
    {
        vk::Buffer buffer {};
        vma::Allocation allocation {};
        void* mapped = nullptr;
    };

    struct CImage
    {
        vk::Image image {};
        vma::Allocation allocation {};
        void* mapped = nullptr;
    };

    struct CUniformBufferObject {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 proj;
    };

    void CreateAllocator();

    CQueueFamilyIndices GetQueueFamilies(vk::PhysicalDevice physicalDevice);
    void RegisterRequestedDeviceExtensions();
    bool InitializeDeviceExtensions(
        vk::PhysicalDevice physicalDevice,
        std::unordered_set<std::string>& enabledExtensions
    );
    CSwapChainInfo GetSwapChainInfo(vk::PhysicalDevice physicalDevice);
    bool isDeviceSuitable(vk::PhysicalDevice physicalDevice);
    vk::PhysicalDevice PickPhysicalDevice();

    void InitializeLogicalDevice();

    vk::SurfaceFormatKHR ChoosetSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& formats);
    vk::PresentModeKHR ChoosePresentMode(const std::vector<vk::PresentModeKHR>& presentModes);
    vk::Extent2D ChooseSwapChainExtent(const vk::SurfaceCapabilitiesKHR& capabilities);
    void CreateSwapChain(
        vk::SurfaceFormatKHR surfaceFormat,
        vk::PresentModeKHR presentMode,
        vk::Extent2D extent
    );

    vk::ImageView CreateImageView(vk::Image image, vk::Format format);
    void CreateImageViews(const std::vector<vk::Image>& images, vk::Format format);

    void CreateRenderPass();

    vk::DescriptorSetLayout CreateDescriptorSetLayout();
    void CreatePipelineLayout(vk::DescriptorSetLayout descriptorSetLayout);
    vk::ShaderModule CreateShaderModule(const std::vector<char>& byteCode);
    void CreatePipeline();

    void CreateFramebuffers();

    void CreateCommandPool();
    void CreateCommandBuffers();

    CBuffer CreateBuffer(
        vk::DeviceSize size,
        vk::BufferUsageFlags usage,
        vk::MemoryPropertyFlags properties,
        vma::AllocationCreateFlags flags
    );

    vk::CommandBuffer BeginSingleTimeCommands();
    void EndSingleTimeCommands(vk::CommandBuffer commandBuffer);

    void CopyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size);

    void CreateVertexBuffer();
    void CreateIndexBuffer();

    void CreateUniformBuffers();

    void CreateDescriptorPool();
    void CreateDescriptorSets();

    CImage CreateImage(
        uint32_t width,
        uint32_t height,
        vk::Format format,
        vk::ImageTiling tiling,
        vk::ImageUsageFlags usage,
        vk::MemoryPropertyFlags properties
    );
    void TransitionImageLayout(
        vk::Image image,
        vk::Format format,
        vk::ImageLayout oldLayout,
        vk::ImageLayout newLayout
    );
    void CopyBufferToImage(
        vk::Buffer buffer,
        vk::Image image,
        uint32_t width,
        uint32_t height
    );
    void CreateTextureImage();
    void CreateTextureImageView(vk::Format format);

    void CreateTextureSampler();

    CVulkanRendererContext m_context {};
    vma::Allocator m_allocator {};

    vk::PhysicalDevice m_physicalDevice {};
    vk::PhysicalDeviceProperties m_physicslDeviceProps {};
    std::unordered_map<std::string, bool> m_requestedDeviceExtensions {};
    std::unordered_set<std::string> m_enabledDeviceExtensions {};
    vk::PhysicalDeviceFeatures m_requestedDeviceFeatures {};

    CQueueFamilyIndices m_queueFamilies {};
    CSwapChainInfo m_swapChainInfo {};

    vk::Device m_device {};

    vk::Queue m_graphicsQueue {};
    vk::Queue m_presentQueue {};

    vk::SwapchainKHR m_swapChain {};
    vk::SurfaceFormatKHR m_surfaceFormat {};
    vk::PresentModeKHR m_presentMode {};
    vk::Extent2D m_swapChainExtent {};

    std::vector<vk::Image> m_images {};
    std::vector<vk::ImageView> m_imageViews {};
    vk::RenderPass m_renderPass {};

    vk::DescriptorSetLayout m_descriptorSetLayout {};
    vk::PipelineLayout m_pipelineLayout {};
    vk::Pipeline m_pipeline {};

    std::vector<vk::Framebuffer> m_frameBuffers {};

    vk::CommandPool m_commandPool {};
    std::vector<vk::CommandBuffer> m_commandBuffers {};
    std::vector<vk::Semaphore> m_imageAvailableSemaphores {};
    std::vector<vk::Semaphore> m_renderFinishedSemaphores {};
    std::vector<vk::Fence> m_inFlightFences {};

    CBuffer m_vertexBuffer {};
    CBuffer m_indexBuffer {};

    std::vector<CBuffer> m_uniformBuffers {};

    vk::DescriptorPool m_descriptorPool {};
    std::vector<vk::DescriptorSet> m_descriptorSets {};

    CImage m_textureImage {};
    vk::ImageView m_textureImageView {};
    vk::Sampler m_textureSampler {};

    uint32_t m_currentFrame = 0;
};
