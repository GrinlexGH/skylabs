#pragma once

#include <optional>

#include "renderapi.hpp"

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>

class CVulkanAPI final : public IRenderApi {
public:
    CVulkanAPI() = default;
    CVulkanAPI(const CVulkanAPI&) = default;
    CVulkanAPI(CVulkanAPI&&) = default;
    CVulkanAPI& operator=(const CVulkanAPI&) = default;
    CVulkanAPI& operator=(CVulkanAPI&&) = default;
    ~CVulkanAPI();

    void Init(IWindow* window) override;
    void Destroy() override;

private:

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
