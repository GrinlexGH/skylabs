#pragma once
#include "../renderer.hpp"

#include "frame_data.hpp"
#include "surface.hpp"
#include "swapchain.hpp"

#include <glm/glm.hpp>

void MoveForward();
void MoveBackward();

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

    static auto TryToCreate(const IWindow* window) -> std::unique_ptr<CRenderer>;
    auto Draw(glm::mat4 view_mat, float deltaTime) -> void override;

private:
    static constexpr int FRAMES_IN_FLIGHT_COUNT = 3;

    CContext m_context { nullptr };

    CSurface m_surface { nullptr };
    CSwapchain m_swapchain { nullptr };

    std::vector<CFrameData> m_frameData;


    vk::raii::PipelineLayout m_pipelineLayout { nullptr };
    vk::raii::Pipeline m_pipeline { nullptr };


    vk::raii::RenderPass m_renderPass { nullptr };
    std::vector<vk::raii::Framebuffer> m_frameBuffers;

    vk::raii::DescriptorSetLayout m_descriptorSetLayout { nullptr };
    vk::raii::DescriptorPool m_descriptorPool { nullptr };
    std::vector<vk::DescriptorSet> m_descriptorSets { nullptr };


    std::vector<vk::raii::Semaphore> m_renderFinishedSemaphores;

    vk::raii::Image m_depthBuffer { nullptr };
    vk::raii::DeviceMemory m_depthBufferMemory { nullptr };
    vk::raii::ImageView m_depthBufferView { nullptr };

    vk::raii::Buffer m_vertexBuffer { nullptr };
    vk::raii::DeviceMemory m_vertexBufferMemory { nullptr };
    vk::raii::Buffer m_indexBuffer { nullptr };
    vk::raii::DeviceMemory m_indexBufferMemory { nullptr };

    std::vector<vk::raii::Buffer> m_uniformBuffers;
    std::vector<vk::raii::DeviceMemory> m_uniformBuffersMemory;
    std::vector<void*> m_uniformBuffersMapped;

    vk::raii::Image m_texture { nullptr };
    vk::raii::DeviceMemory m_textureMemory { nullptr };
    vk::raii::ImageView m_textureView { nullptr };
    vk::raii::Sampler m_textureSampler { nullptr };

    std::uint32_t m_frameIndex = 0;
};
}
