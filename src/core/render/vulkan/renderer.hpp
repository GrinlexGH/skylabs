#pragma once
#include "../renderer.hpp"

#include "frame_data.hpp"
#include "render_context.hpp"
#include "surface.hpp"
#include "swapchain.hpp"
#include <glm/glm.hpp>

namespace Vulkan {
class CRenderer final : public IRenderer
{
public:
    // Window must be valid for the entire lifetime of the renderer
    explicit CRenderer(const IWindow* window);
    CRenderer(const CRenderer&) = delete;
    CRenderer(CRenderer&&) noexcept = default;
    CRenderer& operator=(const CRenderer&) = delete;
    CRenderer& operator=(CRenderer&&) noexcept = default;
    ~CRenderer() override;

    static std::unique_ptr<CRenderer> TryToCreate(const IWindow* window);
    void Draw(glm::mat4 view_mat) override;

private:
    static constexpr int FRAMES_IN_FLIGHT_COUNT = 3;

    std::unique_ptr<CRenderContext> m_context;

    std::unique_ptr<CSurface> m_surface;
    std::unique_ptr<CSwapchain> m_swapchain;

    std::vector<CFrameData> m_frameData;

    vk::raii::RenderPass m_renderPass = VK_NULL_HANDLE;


    std::vector<vk::Framebuffer> m_frameBuffers;

    vk::DescriptorSetLayout m_descriptorSetLayout;
    vk::DescriptorPool m_descriptorPool;
    std::vector<vk::DescriptorSet> m_descriptorSets;

    vk::PipelineLayout m_pipelineLayout;
    vk::Pipeline m_pipeline;

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
}
