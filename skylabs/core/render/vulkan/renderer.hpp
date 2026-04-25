#pragma once
#include <skylabs/core/render/renderer.hpp>

#include <skylabs/core/render/vulkan/in_flight.hpp>
#include <skylabs/core/render/vulkan/platform/surface.hpp>
#include <skylabs/core/render/vulkan/platform/swapchain.hpp>
#include <skylabs/core/render/vulkan/resources/sampler.hpp>
#include <skylabs/core/render/vulkan/resources/buffer.hpp>
#include <skylabs/core/render/vulkan/resources/image.hpp>
#include <skylabs/core/render/vulkan/pipeline/graphics_pipeline.hpp>
#include <skylabs/core/render/vulkan/pipeline/compute_pipeline.hpp>
#include <skylabs/core/render/vulkan/pipeline/pipeline_layout_cache.hpp>
#include <skylabs/core/render/vulkan/pipeline/descriptor_layout_cache.hpp>
#include <skylabs/core/render/vulkan/pipeline/descriptor_allocator.hpp>
#include <skylabs/core/render/vulkan/command_buffer_allocator.hpp>

#include <SDL3_ttf/SDL_ttf.h>

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
    explicit CRenderer(const IWindow* window);
    CRenderer(const CRenderer&) = delete;
    CRenderer(CRenderer&&) noexcept = default;
    CRenderer& operator=(const CRenderer&) = delete;
    CRenderer& operator=(CRenderer&&) noexcept = default;
    ~CRenderer() override;

    static std::unique_ptr<CRenderer> TryToCreate(const IWindow* window);
    void Draw(glm::mat4 viewMat, float fov, float deltaTime) override;

private:
    static constexpr auto FRAMES_IN_FLIGHT_COUNT = 1;
    static constexpr auto GEOMETRY_POOL_SIZE = static_cast<vk::DeviceSize>(128 * 1024 * 1024);

    void HandleSwapchainResult(vk::Result result, std::string_view context);
    void RecreateSwapchain();
    void ResizeTextures();
    void LoadFonts();
    void LoadTextures();
    void LoadModels();

    void UpdateMVP(const glm::mat4& view, float fov);
    void UpdateTextTexture(const CCommandBuffer& cmd, const std::string& text);

    CContext m_context { nullptr };
    CSwapchain m_swapchain { nullptr };

    CPipelineLayoutCache m_pipelineLayoutCache { nullptr };
    CDescriptorLayoutCache m_descriptorLayoutCache { nullptr };
    CDescriptorAllocator m_descriptorAllocator { nullptr };
    CCommandBufferAllocator m_commandBufferAllocator { nullptr };

    InFlightContext m_inFlightContext { nullptr };

    InFlight<CCommandBuffer> m_graphicsCmd { nullptr };

    InFlight<bool> m_firstUse { nullptr };
    InFlight<vk::raii::Fence> m_fence { nullptr };
    InFlight<vk::raii::Semaphore> m_imageAvailableSemaphore { nullptr };

    std::vector<vk::raii::Semaphore> m_renderFinishedSemaphores;

    CBuffer m_stagingBuffer { nullptr };

    CBuffer m_vertexBuffer { nullptr };
    CBuffer m_indexBuffer { nullptr };

    CImage m_matroskinTexture { nullptr };
    CImage m_vikingRoomTexture { nullptr };
    CSampler m_nearestSampler { nullptr };

    InFlight<CImage> m_mainColor { nullptr };
    InFlight<CImage> m_mainColorMSAA { nullptr };
    InFlight<CImage> m_mainDepthMSAA { nullptr };
    InFlight<CBuffer> m_uniform { nullptr };
    InFlight<vk::raii::DescriptorSet> m_mainDescriptorSet { nullptr };
    CGraphicsPipeline m_pipelineMain { nullptr };

    CSampler m_mainSampler { nullptr };
    InFlight<vk::raii::DescriptorSet> m_swapchainDescriptorSet { nullptr };
    CGraphicsPipeline m_pipelineSwapchain { nullptr };

    CSampler m_linearSampler { nullptr };
    InFlight<CImage> m_textTexture { nullptr };
    InFlight<CImage> m_uiColor { nullptr };
    InFlight<vk::raii::DescriptorSet> m_uiDescriptorSet { nullptr };
    CGraphicsPipeline m_pipelineUI { nullptr };

    SubMesh m_matroskin;
    SubMesh m_viking;

    TTF_Font* m_font = nullptr;
    TTF_Font* m_fontUnifont = nullptr;
    TTF_Font* m_fontEmoji = nullptr;
};
}
