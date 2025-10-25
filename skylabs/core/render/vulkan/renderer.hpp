#pragma once
#include <skylabs/core/render/renderer.hpp>

#include <skylabs/core/render/vulkan/frame_data.hpp>
#include <skylabs/core/render/vulkan/surface.hpp>
#include <skylabs/core/render/vulkan/swapchain.hpp>
#include <skylabs/core/render/vulkan/memory/device_buffer.hpp>
#include <skylabs/core/render/vulkan/memory/host_buffer.hpp>
#include <skylabs/core/render/vulkan/memory/image.hpp>

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

    CImage m_depthBuffer { nullptr };

    CDeviceBuffer m_vertexBuffer { nullptr };
    CDeviceBuffer m_indexBuffer { nullptr };

    std::vector<CHostBuffer> m_uniformBuffers;
    std::vector<CMemoryMapping> m_uniformBuffersMapped;

    CImage m_texture { nullptr };
    vk::raii::Sampler m_textureSampler { nullptr };

    std::uint32_t m_frameIndex = 0;
};
}
