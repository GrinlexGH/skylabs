#pragma once

#include "renderer.hpp"

#include "instance.hpp"
#include "../vulkan_window.hpp"

class CVulkanRenderer final : public IRenderer
{
public:
    bool Initialize(IWindow* window) override;
    void Draw() override {}

private:
    IVulkanWindow* m_window = nullptr;

    CVulkanInstance m_instance;
};


#if 0
#include "renderer.hpp"

#include "../vulkan.hpp"
#include "../vulkan_window.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <glm/glm.hpp>
#include <vk_mem_alloc.hpp>

#include <unordered_map>
#include <unordered_set>
#include <optional>

struct CVertex {
    glm::vec3 pos;
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
        attributeDescriptions[0].format = vk::Format::eR32G32B32Sfloat;
        attributeDescriptions[0].offset = offsetof(CVertex, pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = vk::Format::eR32G32B32Sfloat;
        attributeDescriptions[1].offset = offsetof(CVertex, color);

        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = vk::Format::eR32G32Sfloat;
        attributeDescriptions[2].offset = offsetof(CVertex, texCoord);

        return attributeDescriptions;
    }

    bool operator==(const CVertex& other) const {
        return pos == other.pos && color == other.color && texCoord == other.texCoord;
    }
};

namespace std {
    template<> struct hash<CVertex> {
        size_t operator()(CVertex const& vertex) const {
            return ((hash<glm::vec3>()(vertex.pos) ^
                   (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
                   (hash<glm::vec2>()(vertex.texCoord) << 1);
        }
    };
}

struct Particle {
    glm::vec2 position;
    glm::vec2 velocity;
    glm::vec4 color;

    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Particle);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Particle, position);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Particle, color);

        return attributeDescriptions;
    }
};

class CVulkanRenderer final : public IRenderer {
public:
    CVulkanRenderer() = default;
    CVulkanRenderer(const CVulkanRenderer&) = delete;
    CVulkanRenderer(CVulkanRenderer&&) = default;
    CVulkanRenderer& operator=(const CVulkanRenderer&) = delete;
    CVulkanRenderer& operator=(CVulkanRenderer&&) = default;
    ~CVulkanRenderer();

    bool Initialize(IWindow* window) override;
    void Draw() override;

    bool m_frameBufferResized = false;

private:
    void UpdateUniformBuffer(uint32_t currentImage, vk::Extent2D swapChainExtent);
    void LoadModel();

    struct CQueueFamilyIndices
    {
        std::optional<uint32_t> m_graphicsAndCompute;
        std::optional<uint32_t> m_present;

        bool isComplete() const {
            return m_graphicsAndCompute.has_value() && m_present.has_value();
        }
    };

    struct CBuffer
    {
        vk::Buffer buffer {};
        vma::Allocation allocation {};
    };

    struct CImage
    {
        vk::Image image {};
        vma::Allocation allocation {};
    };

    struct CUniformBufferObject {
        alignas(16) glm::mat4 model;
        alignas(16) glm::mat4 view;
        alignas(16) glm::mat4 proj;
    };

    static VKAPI_ATTR vk::Bool32 VKAPI_CALL DebugCallback(
        vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        vk::DebugUtilsMessageTypeFlagBitsEXT messageType,
        const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData
    );

    void _InitializeInstanceExtensions();
    void _InitializeInstance();

    void _SetRequiredDeviceExtensions();
    CQueueFamilyIndices _FindQueueFamilies(vk::PhysicalDevice physicalDevice);
    bool _IsDeviceSuitable(vk::PhysicalDevice physicalDevice);
    void _PickPhysicalDevice();

    void _InitializeDeviceExtensions();
    void _initializeDevice();

    void _CreateAllocator();

    vk::SurfaceFormatKHR _ChooseSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& formats);
    vk::PresentModeKHR _ChoosePresentMode(const std::vector<vk::PresentModeKHR>& presentModes);
    vk::Extent2D _ChooseSwapChainExtent();

    void _CreateSwapchain();
    void _CleanupSwapchain();
    void _RecreateSwapchain();
    vk::ImageView _CreateImageView(vk::Image image, vk::Format format, vk::ImageAspectFlags aspectFlags, uint32_t mipLevels);
    void _CreateImageViews(vk::Format format);

    void _CreateRenderPass();

    void _CreateDescriptorSetLayout();
    vk::ShaderModule _CreateShaderModule(const std::vector<char>& byteCode);
    void _CreatePipeline();
    void _CreateComputePipeline();

    void _CreateColorResources();

    vk::Format _GetSupportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features);
    vk::Format _GetDepthFormat();
    void _CreateDepthResources();

    void _CreateFramebuffers();

    void _CreateCommandPool();
    void _CreateCommandBuffers();

    CBuffer _CreateBuffer(
        vk::DeviceSize size,
        vk::BufferUsageFlags usage,
        vk::MemoryPropertyFlags properties,
        vma::AllocationCreateFlags flags
    );

    vk::CommandBuffer _BeginSingleTimeCommands();
    void _EndSingleTimeCommands(vk::CommandBuffer commandBuffer);

    void _CopyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size);

    void _CreateVertexBuffer();
    void _CreateIndexBuffer();

    void _CreateUniformBuffers();
    void _CreateShaderStorageBuffers();

    void _CreateComputeDescriptorSetLayout();

    void _CreateDescriptorPool();
    void _CreateDescriptorSets();

    void _CreateComputeDescriptorSets();

    CImage _CreateImage(
        uint32_t width,
        uint32_t height,
        uint32_t mipLevels,
        vk::SampleCountFlagBits numSamples,
        vk::Format format,
        vk::ImageTiling tiling,
        vk::ImageUsageFlags usage,
        vk::MemoryPropertyFlags properties
    );
    void _TransitionImageLayout(
        vk::Image image,
        vk::Format format,
        vk::ImageLayout oldLayout,
        vk::ImageLayout newLayout,
        uint32_t mipLevels
    );
    void _CopyBufferToImage(
        vk::Buffer buffer,
        vk::Image image,
        uint32_t width,
        uint32_t height
    );
    void _GenerateMipMaps(vk::Image image, vk::Format format, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);
    void _CreateTextureImage();
    void _CreateTextureImageView(vk::Format format);

    void _CreateTextureSampler();

    void _CreateSyncObjects();

    void _RecordComputeCommandBuffer(vk::CommandBuffer commandBuffer);
    void _CreateComputeCommandBuffers();

    #ifndef NDEBUG
    bool m_enableValidationLayer = true;
    #else
    bool m_enableValidationLayer = false;
    #endif

    IVulkanWindow* m_window = nullptr;

    vma::Allocator m_allocator {};

    std::unordered_map<std::string, bool> m_instanceExtensions {}; // bool is a value that means whether extension is required or not
    std::unordered_set<std::string> m_enabledInstanceExtensions {};
    vk::Instance m_instance {};

    vk::DebugUtilsMessengerEXT m_debugMessenger {};

    vk::PhysicalDevice m_physicalDevice {};
    CQueueFamilyIndices m_queueFamiliesIndices {};

    vk::Device m_device {};
    std::unordered_map<std::string, bool> m_deviceExtensions {};
    std::unordered_set<std::string> m_enabledDeviceExtensions {};

    vk::Queue m_graphicsQueue {};
    vk::Queue m_presentQueue {};
    vk::Queue m_computeQueue {};

    vk::SurfaceCapabilitiesKHR m_surfaceCapabilities {};
    vk::SurfaceFormatKHR m_currentSurfaceFormat {};
    vk::PresentModeKHR m_currentPresentMode {};
    vk::Extent2D m_currentSwapchainExtent {};

    vk::SwapchainKHR m_swapChain {};
    std::vector<vk::Image> m_images {};
    std::vector<vk::ImageView> m_imageViews {};

    CImage m_colorImage {};
    vk::ImageView m_colorImageView {};

    vk::RenderPass m_renderPass {};

    vk::DescriptorSetLayout m_descriptorSetLayout {};
    vk::PipelineLayout m_pipelineLayout {};
    vk::Pipeline m_pipeline {};

    CImage m_depthImage {};
    vk::ImageView m_depthImageView {};

    std::vector<vk::Framebuffer> m_frameBuffers {};

    vk::CommandPool m_commandPool {};
    std::vector<vk::CommandBuffer> m_commandBuffers {};
    std::vector<vk::CommandBuffer> m_computeCommandBuffers {};

    std::vector<CVertex> m_vertices;
    std::vector<uint32_t> m_indices;
    CBuffer m_vertexBuffer {};
    CBuffer m_indexBuffer {};

    std::vector<CBuffer> m_uniformBuffers {};
    std::vector<void*> m_uniformBuffersData {};

    vk::DescriptorSetLayout m_computeDescriptorSetLayout {};
    std::vector<CBuffer> m_shaderStorageBuffers {};

    vk::PipelineLayout m_computePipelineLayout {};
    vk::Pipeline m_computePipeline {};

    vk::DescriptorPool m_descriptorPool {};
    std::vector<vk::DescriptorSet> m_descriptorSets {};
    std::vector<vk::DescriptorSet> m_computeDescriptorSets {};

    uint32_t m_mipLevels = 0;
    CImage m_textureImage {};
    vk::ImageView m_textureImageView {};
    vk::Sampler m_textureSampler {};

    vk::SampleCountFlagBits m_msaaSamples = vk::SampleCountFlagBits::e1;

    std::vector<vk::Semaphore> m_imageAvailableSemaphores {};
    std::vector<vk::Semaphore> m_renderFinishedSemaphores {};
    std::vector<vk::Fence> m_inFlightFences {};
    std::vector<vk::Fence> m_computeInFlightFences;
    std::vector<vk::Semaphore> m_computeFinishedSemaphores;

    uint32_t m_currentFrame = 0;
};
#endif
