#pragma once
#include "skylabs/engine/filesystem.hpp"
#include "skylabs/engine/render/renderer.hpp"
#include "skylabs/engine/render/vulkan/buffer.hpp"
#include "skylabs/engine/render/vulkan/command_buffer.hpp"
#include "skylabs/engine/render/vulkan/command_buffer_allocator.hpp"
#include "skylabs/engine/render/vulkan/context/context.hpp"
#include "skylabs/engine/render/vulkan/descriptor_layout_cache.hpp"
#include "skylabs/engine/render/vulkan/graphics_pipeline.hpp"
#include "skylabs/engine/render/vulkan/in_flight.hpp"
#include "skylabs/engine/render/vulkan/os_adapter.hpp"
#include "skylabs/engine/render/vulkan/pipeline_layout_cache.hpp"
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
    void Draw(glm::mat4 view, float fov, float deltaTime) override;
    void EndFrame() override;

    void OnDeviceReset() override;

private:
    static constexpr auto kFramesInFlightCount = 3;
    static constexpr auto kGeometryPoolSize = static_cast<vk::DeviceSize>(128 * 1024 * 1024);

    void RecreateSwapchain();
    void MarkSurfaceLost();

    Context m_context { nullptr };
    Swapchain m_swapchain { nullptr };
    InFlightContext m_inFlightContext;
    CommandBufferAllocator m_commandBufferAllocator { nullptr };

    // Frame integrity
    bool m_frameActive = false;
    bool m_surfaceLost = false;

    // Frame synchronization
    InFlight<bool> m_firstUse { nullptr };
    InFlight<vk::raii::Fence> m_fence { nullptr };
    InFlight<vk::raii::Semaphore> m_imageAvailableSemaphore { nullptr };
    std::uint32_t m_currentImageIndex = std::numeric_limits<std::uint32_t>::max();

    InFlight<CommandBuffer> m_commandBuffers { nullptr };

    PipelineLayoutCache m_pipelineLayoutCache { nullptr };
    DescriptorLayoutCache m_descriptorLayoutCache { nullptr };

    Buffer m_resourceDescriptorHeap { nullptr };
    vk::DeviceSize m_heapReservedOff = 0;
    vk::DeviceSize m_heapReservedSize = 0;
    vk::DeviceSize m_heapStride = 0;

    InFlight<vma::raii::VirtualAllocation> m_descriptors { nullptr };

    vk::raii::DescriptorPool m_descriptorPool { nullptr };

    InFlight<Buffer> m_viewProjection { nullptr };
    GraphicsPipeline m_pipeline { nullptr };
    InFlight<vk::DescriptorSet> m_descriptorSet { nullptr };
};
}
