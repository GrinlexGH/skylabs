#pragma once
#include <skylabs/core/render/vulkan/context/context.hpp>
#include <skylabs/core/render/vulkan/command_buffer_set.hpp>

namespace Vulkan {
class CFrameData
{
public:
    explicit CFrameData(std::nullptr_t) {}
    explicit CFrameData(const CContext& context);
    CFrameData(const CFrameData&) = delete;
    CFrameData(CFrameData&&) noexcept = default;
    CFrameData& operator=(const CFrameData&) = delete;
    CFrameData& operator=(CFrameData&&) noexcept = default;
    ~CFrameData() = default;

    [[nodiscard]] const std::vector<CCommandBuffer>& GetComputeCommandBuffers() const { return m_computeCommandBuffers.PrimaryBuffers(); }
    [[nodiscard]] const std::vector<CCommandBuffer>& GetGraphicsCommandBuffers() const { return m_graphicsCommandBuffers.PrimaryBuffers(); }
    [[nodiscard]] const vk::raii::Fence& GetFence() const { return m_fence; }
    [[nodiscard]] const vk::raii::Semaphore& GetImageAvailableSemaphore() const { return m_imageAvailableSemaphore; }
    [[nodiscard]] const std::vector<vk::raii::Semaphore>& GetSemaphores() const { return m_semaphores; }

    void RecreateImageAvailableSemaphore() { m_imageAvailableSemaphore = vk::raii::Semaphore { *m_context->Device(), vk::SemaphoreCreateInfo {} }; }

private:
    const CContext* m_context = nullptr;

    CCommandBufferSet m_graphicsCommandBuffers { nullptr };
    CCommandBufferSet m_computeCommandBuffers { nullptr };
    vk::raii::Fence m_fence = nullptr;
    vk::raii::Semaphore m_imageAvailableSemaphore = nullptr;
    std::vector<vk::raii::Semaphore> m_semaphores;
};
}
