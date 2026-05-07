#pragma once
#include <skylabs/core/render/renderer.hpp>
#include <skylabs/core/render/vulkan/renderer_context.hpp>
#include <skylabs/core/render/vulkan/main_pass.hpp>
#include <skylabs/core/render/vulkan/post_process_pass.hpp>

#include <skylabs/core/render/vulkan/platform/surface.hpp>
#include <skylabs/core/render/vulkan/resources/buffer.hpp>
#include <skylabs/core/render/vulkan/resources/image.hpp>
#include <skylabs/core/render/vulkan/pipeline/descriptor_allocator.hpp>
#include <skylabs/core/render/vulkan/command_buffer_allocator.hpp>

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

    CCommandBufferAllocator& CommandBufferAllocator() { return m_commandBufferAllocator; }

private:
    static constexpr auto GEOMETRY_POOL_SIZE = static_cast<vk::DeviceSize>(128 * 1024 * 1024);

    void HandleSwapchainResult(vk::Result result, std::string_view context);
    void RecreateSwapchain();
    void ResizeTextures();
    void LoadTextures();
    void LoadModels();

    void UpdateMVP(const glm::mat4& view, float fov);

    CRendererContext m_rendererContext { nullptr };

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
};
}
