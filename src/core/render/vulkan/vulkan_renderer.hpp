#pragma once
#include "../renderer.hpp"

#include "render_context.hpp"
#include "surface.hpp"
#include "swapchain.hpp"

class CVulkanRenderer final : public IRenderer
{
public:
    // Window must be valid for the entire lifetime of the renderer
    explicit CVulkanRenderer(const IVulkanWindow* window);
    CVulkanRenderer(const CVulkanRenderer&) = delete;
    CVulkanRenderer(CVulkanRenderer&&) noexcept = delete;
    CVulkanRenderer& operator=(const CVulkanRenderer&) = delete;
    CVulkanRenderer& operator=(CVulkanRenderer&&) noexcept = delete;
    ~CVulkanRenderer() override;

    static std::unique_ptr<CVulkanRenderer> TryToCreate(const IVulkanWindow* window);
    void Draw() override;

private:
    std::unique_ptr<Vulkan::CRenderContext> m_context;

    std::unique_ptr<Vulkan::CSurface> m_surface;
    std::unique_ptr<Vulkan::CSwapchain> m_swapchain;
    vk::PipelineLayout m_pipelineLayout;
    std::vector<vk::Framebuffer> m_frameBuffers;
    vk::RenderPass m_renderPass;
    vk::Pipeline m_pipeline;
    vk::CommandPool m_commandPool;

    std::vector<vk::CommandBuffer> m_commandBuffers;
    std::vector<vk::Semaphore> m_currentImageAvailableSemaphores;
    std::vector<vk::Semaphore> m_renderFinishedSemaphores;
    std::vector<vk::Fence> m_inFlightFences;

    const int m_maxFramesInFlight = 25;
    std::uint32_t m_frameIndex = 0;
};
