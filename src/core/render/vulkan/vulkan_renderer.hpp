#pragma once
#include "../renderer.hpp"

#include "frame_data.hpp"
#include "render_context.hpp"
#include "surface.hpp"
#include "swapchain.hpp"
#include <glm/glm.hpp>

class CVulkanRenderer final : public IRenderer
{
public:
    // Window must be valid for the entire lifetime of the renderer
    explicit CVulkanRenderer(const IVulkanWindow* window);
    CVulkanRenderer(const CVulkanRenderer&) = delete;
    CVulkanRenderer(CVulkanRenderer&&) noexcept = default;
    CVulkanRenderer& operator=(const CVulkanRenderer&) = delete;
    CVulkanRenderer& operator=(CVulkanRenderer&&) noexcept = default;
    ~CVulkanRenderer() override;

    static std::unique_ptr<CVulkanRenderer> TryToCreate(const IVulkanWindow* window);
    void Draw(glm::mat4 view_mat) override;

private:
    static constexpr int FRAMES_IN_FLIGHT_COUNT = 3;

    std::unique_ptr<Vulkan::CRenderContext> m_context;

    std::unique_ptr<Vulkan::CSurface> m_surface;
    std::unique_ptr<Vulkan::CSwapchain> m_swapchain;

    std::vector<Vulkan::CFrameData> m_frameData;
    std::vector<vk::Semaphore> m_imageAvailableSemaphores;

    vk::raii::RenderPass m_renderPass = VK_NULL_HANDLE;


    std::vector<vk::Framebuffer> m_frameBuffers;

    vk::DescriptorSetLayout m_descriptorSetLayout;
    vk::DescriptorPool m_descriptorPool;
    std::vector<vk::DescriptorSet> m_descriptorSets;

    vk::PipelineLayout m_pipelineLayout;
    vk::Pipeline m_pipeline;

    vk::CommandPool m_commandPool;
    std::vector<vk::CommandBuffer> m_commandBuffers;

    std::vector<vk::Semaphore> m_renderFinishedSemaphores;

    vk::Buffer m_vertexBuffer;
    vk::DeviceMemory m_vertexBufferMemory;
    vk::Buffer m_indexBuffer;
    vk::DeviceMemory m_indexBufferMemory;

    std::vector<vk::Buffer> m_uniformBuffers;
    std::vector<vk::DeviceMemory> m_uniformBuffersMemory;
    std::vector<void*> m_uniformBuffersMapped;

    vk::Image m_texture;
    vk::DeviceMemory m_textureMemory;
    vk::ImageView m_textureView;
    vk::Sampler m_textureSampler;

    std::uint32_t m_frameIndex = 0;
};
