#pragma once
#include <skylabs/core/render/renderer.hpp>

#include <skylabs/core/render/vulkan/render_graph/resource_manager.hpp>

#include <skylabs/core/render/vulkan/frame_data.hpp>
#include <skylabs/core/render/vulkan/platform/surface.hpp>
#include <skylabs/core/render/vulkan/platform/swapchain.hpp>
#include <skylabs/core/render/vulkan/resources/sampler.hpp>
#include <skylabs/core/render/vulkan/resources/buffer.hpp>
#include <skylabs/core/render/vulkan/resources/image.hpp>
#include <skylabs/core/render/vulkan/pipeline/graphics_pipeline.hpp>

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

    RG::CTextureManager m_textureManager { nullptr };
    RG::CBufferManager m_bufferManager { nullptr };
    RG::CDescriptorManager m_descriptorManager { nullptr };

    RG::BufferHandle m_uniformBuffer;

    RG::TextureHandle m_colorBuffer;
    RG::TextureHandle m_colorBufferMSAAx;
    RG::TextureHandle m_depthBufferMSAAx;
    RG::DescriptorSetHandle m_mainDescriptorSet;
    RG::TextureHandle m_modelTexture;
    RG::BufferHandle m_vertexBuffer;
    RG::BufferHandle m_indexBuffer;

    RG::TextureHandle m_computeBuffer;
    RG::DescriptorSetHandle m_computeDescriptorSet;

    RG::DescriptorSetHandle m_swapchainDescriptorSet;


    RG::TextureHandle m_lightDepth;
    RG::BufferHandle m_lightUBO;
    RG::DescriptorSetHandle m_lightDescriptorSet;
    CGraphicsPipeline m_lightPipeline { nullptr };
    CSampler m_samplerLight { nullptr };




    std::vector<CFrameData> m_frameData;

    vk::raii::CommandPool m_singleCommandPool { nullptr };
    // vk::raii::DescriptorPool m_descriptorPool { nullptr };

    // vk::raii::DescriptorSetLayout m_descriptorSetLayoutMain { nullptr };
    // std::vector<vk::DescriptorSet> m_descriptorSetsMain;
    // CBuffer m_vertexBuffer { nullptr };
    // CBuffer m_indexBuffer { nullptr };
    // CImage m_modelTexture { nullptr };
    CSampler m_modelTextureSampler { nullptr };
    CGraphicsPipeline m_pipelineMain { nullptr };


    // vk::raii::DescriptorSetLayout m_descriptorSetLayoutCompute { nullptr };
    // std::vector<vk::DescriptorSet> m_descriptorSetsCompute;
    vk::raii::PipelineLayout m_computePipelineLayout { nullptr };
    vk::raii::Pipeline m_computePipeline { nullptr };


    // vk::raii::DescriptorSetLayout m_descriptorSetLayoutSwapchain { nullptr };
    // std::vector<vk::DescriptorSet> m_descriptorSetsSwapchain;
    CSampler m_computeSampler { nullptr };
    CSampler m_mainSampler { nullptr };
    CGraphicsPipeline m_pipelineSwapchain { nullptr };
    std::vector<vk::raii::Semaphore> m_renderFinishedSemaphores;

    std::uint32_t m_frameIndex = 0;
};
}
