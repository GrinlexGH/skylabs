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
    static constexpr unsigned int FRAMES_IN_FLIGHT_COUNT = 2;

    void Resize(CFrameData& currentFrameData);
    void UpdateMainDescriptorSets();
    void UpdateComputeDescriptorSets();
    void UpdateSwapchainDescriptorSets();
    void ReleaseComputeBuffers();
    void LoadModelTexture(CHostBuffer& stagingBuffer, const vk::raii::CommandPool& commandPool);
    void LoadModel(CHostBuffer& stagingBuffer, const vk::raii::CommandPool& commandPool);

    CContext m_context { nullptr };

    CSurface m_surface { nullptr };
    CSwapchain m_swapchain { nullptr };

    std::vector<CFrameData> m_frameData;

    vk::raii::CommandPool m_singleCommandPool { nullptr };
    vk::raii::DescriptorPool m_descriptorPool { nullptr };

    vk::raii::DescriptorSetLayout m_descriptorSetLayoutMain { nullptr };
    CDeviceBuffer m_vertexBuffer { nullptr };
    CDeviceBuffer m_indexBuffer { nullptr };
    CImage m_modelTexture { nullptr };
    CSampler m_modelTextureSampler { nullptr };
    std::vector<CHostBuffer> m_uniformBuffers;
    std::vector<vk::DescriptorSet> m_descriptorSetsMain;
    std::vector<CImage> m_colorBuffers;
    std::vector<CImage> m_depthBuffersMSAA;
    std::vector<CImage> m_colorBuffersMSAA;
    CPipeline m_pipelineMain { nullptr };


    vk::raii::DescriptorSetLayout m_descriptorSetLayoutCompute { nullptr };
    std::vector<CImage> m_computeBuffers;
    std::vector<vk::DescriptorSet> m_descriptorSetsCompute;
    vk::raii::PipelineLayout m_computePipelineLayout { nullptr };
    vk::raii::Pipeline m_computePipeline { nullptr };


    vk::raii::DescriptorSetLayout m_descriptorSetLayoutSwapchain { nullptr };
    CSampler m_computeSampler { nullptr };
    CSampler m_mainSampler { nullptr };
    std::vector<vk::DescriptorSet> m_descriptorSetsSwapchain;
    CPipeline m_pipelineSwapchain { nullptr };
    std::vector<vk::raii::Semaphore> m_renderFinishedSemaphores;

    std::uint32_t m_frameIndex = 0;
};
}
