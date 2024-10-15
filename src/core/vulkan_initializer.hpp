#pragma once
#include "window.hpp"

#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>
#include <optional>
#include <vector>
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_handles.hpp>
#include <vulkan/vulkan_structs.hpp>

namespace vk_initializer {
    extern bool enableValidationLayers;

    constexpr std::size_t MAX_FRAMES_IN_FLIGHT = 3;

    const std::vector<const char*> g_validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    const std::vector<const char*> g_deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    struct CQueueFamilyIndices {
        std::optional<uint32_t> m_graphicsFamily;
        std::optional<uint32_t> m_presentFamily;

        bool isComplete() const {
            return m_graphicsFamily.has_value() && m_presentFamily.has_value();
        }
    };

    struct CSwapChainSupportDetails {
        vk::SurfaceCapabilitiesKHR m_capabilities;
        std::vector<vk::SurfaceFormatKHR> m_formats;
        std::vector<vk::PresentModeKHR> m_presentModes;
    };

    struct CVertex {
        glm::vec2 pos;
        glm::vec3 color;

        static vk::VertexInputBindingDescription getBindingDescription() {
            vk::VertexInputBindingDescription bindingDescription{};
            bindingDescription.binding = 0;
            bindingDescription.stride = sizeof(CVertex);
            bindingDescription.inputRate = vk::VertexInputRate::eVertex;

            return bindingDescription;
        }

        static std::array<vk::VertexInputAttributeDescription, 2> getAttributeDescriptions() {
            std::array<vk::VertexInputAttributeDescription, 2> attributeDescriptions {};
            attributeDescriptions[0].binding = 0;
            attributeDescriptions[0].location = 0;
            attributeDescriptions[0].format = vk::Format::eR32G32Sfloat;
            attributeDescriptions[0].offset = offsetof(CVertex, pos);
            attributeDescriptions[1].binding = 0;
            attributeDescriptions[1].location = 1;
            attributeDescriptions[1].format = vk::Format::eR32G32B32Sfloat;
            attributeDescriptions[1].offset = offsetof(CVertex, color);

            return attributeDescriptions;
        }
    };

    struct CUniformBufferObject {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 proj;
    };

    extern const std::vector<CVertex> g_vertices;
    extern const std::vector<uint16_t> g_indices;

    vk::Instance CreateInstance(vk::DebugUtilsMessengerEXT &debugMessenger);
    vk::SurfaceKHR CreateSurface(vk::Instance instance, IWindow* window);
    vk::PhysicalDevice PickPhysicalDevice(vk::Instance instance, vk::SurfaceKHR surface);
    vk::Device CreateLogicalDevice(
        vk::PhysicalDevice physicalDevice,
        CQueueFamilyIndices queueIndices
    );
    vk::SwapchainKHR CreateSwapChain(
        vk::Device device,
        vk::SurfaceKHR surface,
        const CQueueFamilyIndices& queueIndices,
        const CSwapChainSupportDetails& swapChainSupport,
        vk::SurfaceFormatKHR surfaceFormat,
        vk::PresentModeKHR presentMode,
        vk::Extent2D extent
    );
    std::vector<vk::ImageView> CreateImageViews(
        vk::Device device,
        const std::vector<vk::Image>& images,
        vk::Format imageFormat
    );
    vk::RenderPass CreateRenderPass(vk::Device device, vk::Format imageFormat);
    vk::PipelineLayout CreatePipelineLayout(vk::Device device, vk::DescriptorSetLayout& descriptorSetLayout);
    vk::Pipeline CreatePipeline(
        vk::Device device,
        vk::PipelineLayout pipelineLayout,
        vk::RenderPass renderPass
    );
    std::vector<vk::Framebuffer> CreateFramebuffers(
        vk::Device device,
        const std::vector<vk::ImageView>& imageViews,
        vk::RenderPass renderPass,
        vk::Extent2D extent
    );
    vk::CommandPool CreateCommandPool(vk::Device device, uint32_t graphicsFamilyIndex);
    std::vector<vk::CommandBuffer> CreateCommandBuffers(vk::Device device, vk::CommandPool commandPool);
    vk::Buffer CreateVertexBuffer(
        vk::PhysicalDevice physicalDevice,
        vk::Device device,
        vk::CommandPool commandPool,
        vk::Queue graphicsQueue,
        vk::DeviceMemory& vertexBufferMemory
    );
    vk::Buffer CreateIndexBuffer(
        vk::PhysicalDevice physicalDevice,
        vk::Device device,
        vk::CommandPool commandPool,
        vk::Queue graphicsQueue,
        vk::DeviceMemory& indexBufferMemory
    );
    std::vector<vk::Buffer> CreateUniformBuffers(
        vk::PhysicalDevice physicalDevice,
        vk::Device device,
        std::vector<vk::DeviceMemory>& uniformBuffersMemory,
        std::vector<void*>& uniformBuffersMapped
    );
    vk::DescriptorSetLayout CreateDescriptorSetLayout(vk::Device device);
    vk::DescriptorPool CreateDescriptorPool(vk::Device device);
    std::vector<vk::DescriptorSet> CreateDescriptorSets(
        vk::Device device,
        vk::DescriptorSetLayout descriptorSetLayout,
        vk::DescriptorPool descriptorPool,
        std::vector<vk::Buffer> uniformBuffers
    );

    VKAPI_ATTR vk::Bool32 VKAPI_CALL DebugCallback(
        vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        vk::DebugUtilsMessageTypeFlagBitsEXT messageType,
        const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData
    );

    std::vector<std::string_view> FindMissingLayers(const std::vector<vk::LayerProperties>& availableLayers, const std::vector<const char*>& neededLayers);
    std::vector<std::string_view> FindMissingExtensions(const std::vector<vk::ExtensionProperties>& availableExts, const std::vector<const char*>& neededExts);
    std::vector<const char*> GetRequiredInstanceExtensions();

    bool isDeviceSuitable(vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface);
    CQueueFamilyIndices FindQueueFamilies(vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface);
    CSwapChainSupportDetails QuerySwapChainSupport(vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface);

    vk::SurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats);
    vk::PresentModeKHR ChooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes);
    vk::Extent2D ChooseSwapExtent(IWindow* window, const vk::SurfaceCapabilitiesKHR& capabilities);

    vk::ShaderModule CreateShaderModule(vk::Device device, const std::vector<char>& byteCode);

    uint32_t FindMemoryType(vk::PhysicalDevice physicalDevice, uint32_t typeFilter, vk::MemoryPropertyFlags properties);

    vk::Buffer CreateBuffer(
        vk::PhysicalDevice physicalDevice,
        vk::Device device,
        vk::DeviceSize size,
        vk::BufferUsageFlags usage,
        vk::MemoryPropertyFlags properties,
        vk::DeviceMemory& bufferMemory
    );
    void CopyBuffer(
        vk::Device device,
        vk::CommandPool commandPool,
        vk::Buffer srcBuffer,
        vk::Buffer dstBuffer,
        vk::DeviceSize size
    );
};
