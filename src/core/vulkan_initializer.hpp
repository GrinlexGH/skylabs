#pragma once
#include <vulkan/vulkan.hpp>
#include <optional>
#include <vector>

#include "window.hpp"

namespace VulkanInitializer {
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
        vk::SurfaceKHR surface,
        vk::Queue& graphicsQueue,
        vk::Queue& presentQueue
    );
    vk::SwapchainKHR CreateSwapChain(
        vk::PhysicalDevice physicalDevice,
        vk::Device device,
        vk::SurfaceKHR surface,
        IWindow* window,
        std::vector<vk::Image>& swapChainImages,
        vk::Format& swapChainImageFormat,
        vk::Extent2D& swapChainExtent
    );
    std::vector<vk::ImageView> CreateImageViews(
        vk::Device device,
        const std::vector<vk::Image>& swapChainImages,
        vk::Format swapChainImageFormat
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

    bool isDeviceSuitable(vk::PhysicalDevice device, vk::SurfaceKHR surface);
    CQueueFamilyIndices FindQueueFamilies(vk::PhysicalDevice device, vk::SurfaceKHR surface);
    CSwapChainSupportDetails QuerySwapChainSupport(vk::PhysicalDevice device, vk::SurfaceKHR surface);

    vk::SurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats);
    vk::PresentModeKHR ChooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes);
    vk::Extent2D ChooseSwapExtent(IWindow* window, const vk::SurfaceCapabilitiesKHR& capabilities);
};
