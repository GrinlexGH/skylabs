#pragma once

#include <optional>

#include "renderapi.hpp"

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>

class CVulkanAPI final : public IRenderApi {
public:
    CVulkanAPI()                                = default;
    CVulkanAPI(const CVulkanAPI&)               = default;
    CVulkanAPI(CVulkanAPI&&)                    = default;
    CVulkanAPI& operator=(const CVulkanAPI&)    = default;
    CVulkanAPI& operator=(CVulkanAPI&&)         = default;
    ~CVulkanAPI();

    void Init(IWindow* window)  override;
    void Destroy()              override;

private:
    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily_;
        std::optional<uint32_t> presentFamily_;

        bool isComplete() {
            return graphicsFamily_.has_value() && presentFamily_.has_value();
        }
    };

    struct SwapChainSupportDetails {
        vk::SurfaceCapabilitiesKHR capabilities_;
        std::vector<vk::SurfaceFormatKHR> formats_;
        std::vector<vk::PresentModeKHR> presentModes_;
    };

    void CreateInstance();
    void SetupDebugMessenger();
    void CreateSurface(IWindow* window);
    void PickPhysicalDevice();
    void CreateLogicalDevice();
    void CreateSwapChain(IWindow* window);

    bool CheckValidationLayerSupport();
    std::vector<const char*> GetRequiredExtensions();

    void PopulateDebugMessengerCreateInfo           (vk::DebugUtilsMessengerCreateInfoEXT& createInfo);

    bool isDeviceSuitable                           (vk::PhysicalDevice device);
    QueueFamilyIndices FindQueueFamilies            (vk::PhysicalDevice device) const;
    bool CheckDeviceExtensionSupport                (vk::PhysicalDevice device);
    SwapChainSupportDetails QuerySwapChainSupport   (vk::PhysicalDevice device) const;

    bool CheckDeviceValidationLayerSupport          (vk::PhysicalDevice device);

    vk::SurfaceFormatKHR ChooseSwapSurfaceFormat    (const std::vector<vk::SurfaceFormatKHR>& availableFormats);
    vk::PresentModeKHR ChooseSwapPresentMode        (const std::vector<vk::PresentModeKHR>& availablePresentModes);
    vk::Extent2D ChooseSwapExtent                   (IWindow* window, const vk::SurfaceCapabilitiesKHR& capabilities);

    void CreateImageViews();

    bool initialized_ = false;
    vk::Instance instance_;
    vk::DebugUtilsMessengerEXT debugMessenger_;
    vk::SurfaceKHR surface_;
    vk::PhysicalDevice physicalDevice_;
    vk::Device device_;
    vk::Queue graphicsQueue_;
    vk::Queue presentQueue_;
    vk::SwapchainKHR swapChain_;
    std::vector<vk::Image> swapChainImages_;
    vk::Format swapChainImageFormat_;
    vk::Extent2D swapChainExtent_;
    std::vector<vk::ImageView> swapChainImageViews_;
};
