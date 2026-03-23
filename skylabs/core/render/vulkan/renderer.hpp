#pragma once
#include <skylabs/core/render/renderer.hpp>

#include <skylabs/core/render/vulkan/render_graph/graph.hpp>
#include <skylabs/core/render/vulkan/render_graph/descriptor_pool.hpp>

#include <skylabs/core/render/vulkan/frame_data.hpp>
#include <skylabs/core/render/vulkan/platform/surface.hpp>
#include <skylabs/core/render/vulkan/platform/swapchain.hpp>
#include <skylabs/core/render/vulkan/resources/sampler.hpp>
#include <skylabs/core/render/vulkan/resources/buffer.hpp>
#include <skylabs/core/render/vulkan/resources/image.hpp>
#include <skylabs/core/render/vulkan/pipeline/graphics_pipeline.hpp>
#include <skylabs/core/render/vulkan/pipeline/compute_pipeline.hpp>
#include <skylabs/core/render/vulkan/pipeline/pipeline_layout_cache.hpp>

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

    static std::unique_ptr<CRenderer> TryToCreate(const IWindow* window);
    void Draw(glm::mat4 viewMat, float deltaTime) override;

private:
    static constexpr unsigned int FRAMES_IN_FLIGHT_COUNT = 3;

    void Resize(CFrameData& currentFrameData);
    void UpdateMainDescriptorSets();
    void UpdateComputeDescriptorSets();
    void UpdateSwapchainDescriptorSets();
    void ReleaseComputeBuffers();
    void LoadModelTexture(CBuffer& stagingBuffer, const vk::raii::CommandPool& commandPool);
    void LoadModel(CBuffer& stagingBuffer, const vk::raii::CommandPool& commandPool);

    CContext m_context { nullptr };

    CSurface m_surface { nullptr };
    CSwapchain m_swapchain { nullptr };

    RG::CRenderGraph m_graph { nullptr };

    CPipelineLayoutCache m_pipelineLayoutCache { nullptr };

    CTexturePool m_textureManager { nullptr };
    CBufferPool m_bufferManager { nullptr };
    CDescriptorPool m_descriptorManager { nullptr };

    BufferHandle m_uniformBuffer;

    TextureHandle m_colorBuffer;
    TextureHandle m_colorBufferMSAAx;
    TextureHandle m_depthBufferMSAAx;
    DescriptorSetHandle m_mainDescriptorSet;
    TextureHandle m_modelTexture;
    BufferHandle m_vertexBuffer;
    BufferHandle m_indexBuffer;

    TextureHandle m_computeBuffer;
    DescriptorSetHandle m_computeDescriptorSet;

    DescriptorSetHandle m_swapchainDescriptorSet;


    TextureHandle m_lightDepth;
    BufferHandle m_lightUBO;
    DescriptorSetHandle m_lightDescriptorSet;
    CGraphicsPipeline m_lightPipeline { nullptr };
    CSampler m_samplerLight { nullptr };




    std::vector<CFrameData> m_frameData;
    vk::raii::CommandPool m_singleCommandPool { nullptr };

    CSampler m_modelTextureSampler { nullptr };
    CGraphicsPipeline m_pipelineMain { nullptr };


    CComputePipeline m_computePipeline { nullptr };


    CSampler m_computeSampler { nullptr };
    CSampler m_mainSampler { nullptr };
    CGraphicsPipeline m_pipelineSwapchain { nullptr };
    std::vector<vk::raii::Semaphore> m_renderFinishedSemaphores;

    std::uint32_t m_frameIndex = 0;
};
}
