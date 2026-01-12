#pragma once
#include <skylabs/core/render/renderer.hpp>

#include <skylabs/core/render/vulkan/frame_data.hpp>
#include <skylabs/core/render/vulkan/surface.hpp>
#include <skylabs/core/render/vulkan/swapchain.hpp>
#include <skylabs/core/render/vulkan/sampler.hpp>
#include <skylabs/core/render/vulkan/memory/device_buffer.hpp>
#include <skylabs/core/render/vulkan/memory/host_buffer.hpp>
#include <skylabs/core/render/vulkan/memory/image.hpp>
#include <skylabs/core/render/vulkan/pipeline.hpp>
#include <skylabs/core/render/vulkan/legacy_render_pass.hpp>

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
    static constexpr unsigned int FRAMES_IN_FLIGHT_COUNT = 3;

    CContext m_context { nullptr };

    CSurface m_surface { nullptr };
    CSwapchain m_swapchain { nullptr };

    std::vector<CFrameData> m_frameData;

    CPipeline m_pipelineMain { nullptr };
    CPipeline m_pipelineSwapchain { nullptr };

    CLegacyRenderPass m_mainPass { nullptr };
    CLegacyRenderPass m_swapchainPass { nullptr };

    std::vector<vk::raii::Framebuffer> m_frameBuffersSwapchain;

    CSampler m_mainSampler { nullptr };

    vk::raii::DescriptorSetLayout m_descriptorSetLayoutMain { nullptr };
    vk::raii::DescriptorPool m_descriptorPoolMain { nullptr };
    std::vector<vk::DescriptorSet> m_descriptorSetsMain;

    vk::raii::DescriptorSetLayout m_descriptorSetLayoutSwapchain { nullptr };
    vk::raii::DescriptorPool m_descriptorPoolSwapchain { nullptr };
    std::vector<vk::DescriptorSet> m_descriptorSetsSwapchain;


    std::vector<vk::raii::Semaphore> m_renderFinishedSemaphores;

    CImage m_colorBuffer { nullptr };
    CImage m_depthBufferMSAA { nullptr };
    CImage m_colorBufferMSAA { nullptr };

    CDeviceBuffer m_vertexBuffer { nullptr };
    CDeviceBuffer m_indexBuffer { nullptr };

    std::vector<CHostBuffer> m_uniformBuffers;
    std::vector<CBufferMapping> m_uniformBuffersMapped;

    CImage m_modelTexture { nullptr };
    CSampler m_modelTextureSampler { nullptr };


    std::uint32_t m_frameIndex = 0;
};
}
