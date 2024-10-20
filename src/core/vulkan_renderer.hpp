#pragma once

#include "renderer.hpp"

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>

class CVulkanRenderer final : public IRenderer {
public:
    CVulkanRenderer() = default;
    CVulkanRenderer(const CVulkanRenderer&) = default;
    CVulkanRenderer(CVulkanRenderer&&) = default;
    CVulkanRenderer& operator=(const CVulkanRenderer&) = default;
    CVulkanRenderer& operator=(CVulkanRenderer&&) = default;
    ~CVulkanRenderer();

    void Initialize(IWindow* window) override;
    void Draw();
    void Destroy() override;

    bool m_frameBufferResized = false;

private:
    void RecreateSwapChain();
    void CleanupSwapChain();
    void UpdateUniformBuffer(uint32_t currentImage, vk::Extent2D swapChainExtent);

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
    vk::DescriptorSetLayout m_descriptorSetLayout {};
    vk::PipelineLayout m_pipelineLayout {};
    vk::Pipeline m_pipeline {};
    std::vector<vk::Framebuffer> m_frameBuffers {};

    // todo: combine vertex buffer into a struct
    vk::Buffer m_vertexBuffer {};
    vk::DeviceMemory m_vertexBufferMemory {};
    vk::Buffer m_indexBuffer {};
    vk::DeviceMemory m_indexBufferMemory {};
    std::vector<vk::Buffer> m_uniformBuffers;
    std::vector<vk::DeviceMemory> m_uniformBuffersMemory;
    std::vector<void*> m_uniformBuffersMapped;
    vk::DescriptorPool m_descriptorPool;
    std::vector<vk::DescriptorSet> m_descriptorSets;

    vk::CommandPool m_commandPool {};
    std::vector<vk::CommandBuffer> m_commandBuffers {};
    std::vector<vk::Semaphore> m_imageAvailableSemaphores {};
    std::vector<vk::Semaphore> m_renderFinishedSemaphores {};
    std::vector<vk::Fence> m_inFlightFences {};

    vk::MemoryRequirements m_memRequirements {};

    IWindow* m_window = nullptr;

    uint32_t m_currentFrame = 0;
};
