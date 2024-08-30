#pragma once

#include "renderapi.hpp"

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>

class CVulkanRenderer final : public IRenderApi {
public:
    CVulkanRenderer() = default;
    CVulkanRenderer(const CVulkanRenderer&) = default;
    CVulkanRenderer(CVulkanRenderer&&) = default;
    CVulkanRenderer& operator=(const CVulkanRenderer&) = default;
    CVulkanRenderer& operator=(CVulkanRenderer&&) = default;
    ~CVulkanRenderer();

    void Init(IWindow* window) override;
    void Draw();
    void WaitIdle();
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
    std::vector<vk::Image> m_images {};
    vk::Format m_imageFormat {};
    vk::Extent2D m_swapChainExtent {};
    std::vector<vk::ImageView> m_imageViews {};
    vk::RenderPass m_renderPass {};
    vk::PipelineLayout m_pipelineLayout {};
    vk::Pipeline m_pipeline {};
    std::vector<vk::Framebuffer> m_frameBuffers {};
    vk::CommandPool m_commandPool {};
    vk::CommandBuffer m_commandBuffer {};
    vk::Semaphore m_imageAvailableSemaphore {};
    vk::Semaphore m_renderFinishedSemaphore {};
    vk::Fence m_inFlightFence {};
};
