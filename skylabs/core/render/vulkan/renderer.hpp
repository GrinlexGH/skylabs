#pragma once
#include <skylabs/core/render/renderer.hpp>

#include <skylabs/core/render/vulkan/render_graph/graph.hpp>
#include <skylabs/core/render/vulkan/render_graph/descriptor_pool.hpp>

#include <skylabs/core/render/vulkan/frame.hpp>
#include <skylabs/core/render/vulkan/platform/surface.hpp>
#include <skylabs/core/render/vulkan/platform/swapchain.hpp>
#include <skylabs/core/render/vulkan/resources/sampler.hpp>
#include <skylabs/core/render/vulkan/resources/buffer.hpp>
#include <skylabs/core/render/vulkan/resources/image.hpp>
#include <skylabs/core/render/vulkan/pipeline/graphics_pipeline.hpp>
#include <skylabs/core/render/vulkan/pipeline/compute_pipeline.hpp>
#include <skylabs/core/render/vulkan/pipeline/pipeline_layout_cache.hpp>

struct SubMesh {
    std::uint32_t indexCount = 0;
    vma::raii::VirtualAllocation vtxAlloc = nullptr;
    vma::raii::VirtualAllocation idxAlloc = nullptr;

    vk::DeviceSize VtxOffset() const { return vtxAlloc.getInfo().offset; }
    vk::DeviceSize IdxOffset() const { return idxAlloc.getInfo().offset; }
};

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
    static constexpr unsigned int FRAMES_IN_FLIGHT_COUNT = 4;
    static constexpr vk::DeviceSize GEOMETRY_POOL_SIZE = 128 * 1024 * 1024;

    void Resize(CFrame& currentFrameData);
    void LoadModelTextures(CBuffer& stagingBuffer, const vk::raii::CommandPool& commandPool);
    void LoadModels(CBuffer& stagingBuffer, const vk::raii::CommandPool& commandPool);

    CContext m_context { nullptr };

    CSwapchain m_swapchain { nullptr };

    CCommandBufferSet m_graphicsCommands { nullptr };
    CCommandBufferSet m_computeCommands { nullptr };

    CPipelineLayoutCache m_pipelineLayoutCache { nullptr };

    std::vector<CFrame> m_frameData;
    vk::raii::CommandPool m_singleCommandPool { nullptr };

    CTexturePool m_textureManager { nullptr };
    CBufferPool m_bufferManager { nullptr };
    CDescriptorPool m_descriptorManager { nullptr };

    SubMesh m_matroskin;
    SubMesh m_viking;

    // Main pipeline
    BufferHandle m_uniformBuffer;
    TextureHandle m_matroskinModelTexture;
    TextureHandle m_roomModelTexture;
    CSampler m_modelTextureSampler { nullptr };

    BufferHandle m_vertexBuffer;
    BufferHandle m_indexBuffer;

    TextureHandle m_colorBuffer;
    TextureHandle m_colorBufferMSAAx;
    TextureHandle m_depthBufferMSAAx;

    DescriptorSetHandle m_mainDescriptorSetMatroskin;
    DescriptorSetHandle m_mainDescriptorSetVikingRoom;
    CGraphicsPipeline m_pipelineMain { nullptr };

    // Final swapchain pipeline
    CSampler m_mainSampler { nullptr };

    DescriptorSetHandle m_swapchainDescriptorSet;
    CGraphicsPipeline m_pipelineSwapchain { nullptr };

    std::vector<vk::raii::Semaphore> m_renderFinishedSemaphores;

    std::uint32_t m_frameIndex = 0;
};
}
