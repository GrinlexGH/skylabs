#pragma once
#include <skylabs/core/render/vulkan/main_pass.hpp>
#include <skylabs/core/render/renderer.hpp>
#include <skylabs/core/render/vulkan/post_process_pass.hpp>
#include <skylabs/core/render/vulkan/in_flight.hpp>
#include <skylabs/core/render/vulkan/pipeline/pipeline_layout_cache.hpp>
#include <skylabs/core/render/vulkan/pipeline/descriptor_layout_cache.hpp>
#include <skylabs/core/render/vulkan/pipeline/descriptor_allocator.hpp>
#include <skylabs/core/render/vulkan/platform/swapchain.hpp>
#include <skylabs/core/render/vulkan/command_recording/command_buffer_allocator.hpp>
#include <skylabs/core/render/vulkan/render_object.hpp>
#include <skylabs/core/render/render_object.hpp>
#include <skylabs/core/render/vulkan/resources/image.hpp>
#include <skylabs/public/window.hpp>

namespace Vulkan {
class CRenderer final : public IRenderer
{
public:
    explicit CRenderer(const IWindow* window, const IOSConnector* surfaceProvider);
    CRenderer(const CRenderer&) = delete;
    CRenderer(CRenderer&&) = delete;
    CRenderer& operator=(const CRenderer&) = delete;
    CRenderer& operator=(CRenderer&&) = delete;
    ~CRenderer() override;

    static std::unique_ptr<CRenderer> TryToCreate(const IWindow* window, const IOSConnector* surfaceProvider);
    void Draw(glm::mat4 viewMat, float fov, float deltaTime) override;

    std::uint32_t UploadMesh(const std::vector<CVertex>& vertices, const std::vector<std::uint16_t>& indices);

    ::CRenderObject& GetObjectData(std::uint32_t id);
    const ::CRenderObject& GetObjectData(std::uint32_t id) const;
    CRenderObject UploadGameObject(std::uint32_t meshId, const glm::mat4& matrix, std::uint16_t colorID);

    CCommandBufferAllocator& CommandBufferAllocator() { return m_commandBufferAllocator; }

private:
    static constexpr auto FRAMES_IN_FLIGHT_COUNT = 3;
    static constexpr auto GEOMETRY_POOL_SIZE = static_cast<vk::DeviceSize>(128 * 1024 * 1024);

    void RecreateSwapchain();
    void ResizeTextures();
    void LoadTextures();
    void LoadModels();
    void LoadObjects();

    void UpdateMVP(const glm::mat4& view, float fov);

    CreationTools GetCreationTools() {
        return {
            m_context,
            m_inFlightContext,
            m_pipelineLayoutCache,
            m_descriptorLayoutCache,
            m_descriptorAllocator
        };
    }

    CContext m_context { nullptr };

    CSwapchain m_swapchain { nullptr };
    CInFlightContext m_inFlightContext;

    CPipelineLayoutCache m_pipelineLayoutCache { nullptr };
    CDescriptorLayoutCache m_descriptorLayoutCache { nullptr };
    CDescriptorAllocator m_descriptorAllocator { nullptr };

    CCommandBufferAllocator m_commandBufferAllocator { nullptr };
    InFlight<CCommandBuffer> m_graphicsCmd { nullptr };

    InFlight<bool> m_firstUse { nullptr };
    InFlight<vk::raii::Fence> m_fence { nullptr };
    InFlight<vk::raii::Semaphore> m_imageAvailableSemaphore { nullptr };

    std::vector<vk::raii::Semaphore> m_renderFinishedSemaphores;

    CBuffer m_stagingBuffer { nullptr };

    CBuffer m_vertexBuffer { nullptr };
    CBuffer m_indexBuffer { nullptr };

    std::vector<CImage> m_meshTextures;
    std::vector<SubMesh> m_meshes;

    CMainPass m_mainPass { nullptr };
    CPostProcessPass m_postProcessPass { nullptr };

    std::vector<::CRenderObject> m_objects;
};
}
