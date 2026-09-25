#pragma once
#include "skylabs/engine/filesystem.hpp"
#include "skylabs/engine/render/renderer.hpp"
#include "skylabs/engine/render/vulkan/context/context.hpp"
#include "skylabs/engine/render/vulkan/in_flight.hpp"
#include "skylabs/engine/render/vulkan/os_adapter.hpp"
#include "skylabs/engine/render/vulkan/swapchain.hpp"
#include "skylabs/engine/window.hpp"

namespace sk::render::vulkan {
class Renderer final : public IRenderer {
public:
    explicit Renderer(const IWindow* window, const IOSAdapter* osAdapter,
                      const filesystem::Filesystem& filesystem);
    Renderer(const Renderer&) = delete;
    Renderer(Renderer&&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    Renderer& operator=(Renderer&&) = delete;
    ~Renderer() override;

    void BeginFrame() override;
    void Draw(glm::mat4 viewMat, float fov, float deltaTime) override;
    void EndFrame() override;

    void OnPossibleSwapchainResize() override;

private:
    static constexpr auto kFramesInFlightCount = 3;
    static constexpr auto kGeometryPoolSize = static_cast<vk::DeviceSize>(128 * 1024 * 1024);

    void RecreateSwapchain();

    Context m_context { nullptr };
    Swapchain m_swapchain { nullptr };
    InFlightContext m_inFlightContext;

    // Frame synchronization
    InFlight<bool> m_firstUse { nullptr };
    InFlight<vk::raii::Fence> m_fence { nullptr };
    InFlight<vk::raii::Semaphore> m_imageAvailableSemaphore { nullptr };
    std::vector<vk::raii::Semaphore> m_imageRenderFinishedSemaphores;
};
}
