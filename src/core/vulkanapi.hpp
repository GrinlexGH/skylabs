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

    bool m_initialized = false;
    vk::Instance m_instance {};
    vk::DebugUtilsMessengerEXT m_debugMessenger {};
    vk::SurfaceKHR m_surface {};
    vk::PhysicalDevice m_physicalDevice {};
    vk::Device m_device {};
    vk::Queue m_graphicsQueue {};
    vk::Queue m_presentQueue {};
    vk::SwapchainKHR m_swapChain {};
    std::vector<vk::Image> m_swapChainImages {};
    vk::Format m_swapChainImageFormat {};
    vk::Extent2D m_swapChainExtent {};
    std::vector<vk::ImageView> m_swapChainImageViews {};
};
