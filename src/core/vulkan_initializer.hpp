#pragma once
#include "window.hpp"

#include <vulkan/vulkan.hpp>
#include <optional>
#include <vector>

namespace vulkan_initializer {
    #ifdef NDEBUG
        extern bool enableValidationLayers;
    #else
        extern bool enableValidationLayers;
    #endif

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
        CQueueFamilyIndices queueIndices,
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
    vk::PipelineLayout CreatePipelineLayout(vk::Device device);
    vk::Pipeline CreatePipeline(
        vk::Device device,
        vk::PipelineLayout pipelineLayout,
        vk::RenderPass renderPass
    );
    std::vector<vk::Framebuffer> CreateFramebuffers(
        vk::Device device,
        std::vector<vk::ImageView> imageViews,
        vk::RenderPass renderPass,
        vk::Extent2D extent
    );
    vk::CommandPool CreateCommandPool(vk::Device device, CQueueFamilyIndices queueIndices);
    vk::CommandBuffer CreateCommandBuffer(vk::Device device, vk::CommandPool commandPool);

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
};
