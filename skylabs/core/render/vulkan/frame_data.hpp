#pragma once
#include <skylabs/core/render/vulkan/context/context.hpp>

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

    [[nodiscard]] const vk::raii::CommandPool& GetComputeCommandPool() const { return m_computeCommandPool; }
    [[nodiscard]] const vk::raii::CommandBuffers& GetComputeCommandBuffers() const { return m_computeCommandBuffers; }
    [[nodiscard]] const vk::raii::CommandPool& GetGraphicsCommandPool() const { return m_graphicsCommandPool; }
    [[nodiscard]] const vk::raii::CommandBuffers& GetGraphicsCommandBuffers() const { return m_graphicsCommandBuffers; }
    [[nodiscard]] const vk::raii::Fence& GetFence() const { return m_fence; }
    [[nodiscard]] const vk::raii::Semaphore& GetImageAvailableSemaphore() const { return m_imageAvailableSemaphore; }
    [[nodiscard]] const std::vector<vk::raii::Semaphore>& GetSemaphores() const { return m_semaphores; }

    void RecreateImageAvailableSemaphore() { m_imageAvailableSemaphore = vk::raii::Semaphore { *m_context->Device(), vk::SemaphoreCreateInfo {} }; }

private:
    const CContext* m_context = nullptr;

    vk::raii::CommandPool m_computeCommandPool = nullptr;
    vk::raii::CommandBuffers m_computeCommandBuffers = nullptr;
    vk::raii::CommandPool m_graphicsCommandPool = nullptr;
    vk::raii::CommandBuffers m_graphicsCommandBuffers = nullptr;
    vk::raii::Fence m_fence = nullptr;
    vk::raii::Semaphore m_imageAvailableSemaphore = nullptr;
    std::vector<vk::raii::Semaphore> m_semaphores;
};
}
