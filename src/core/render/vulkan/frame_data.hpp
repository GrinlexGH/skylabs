#pragma once
#include "render_context.hpp"

namespace Vulkan {
class CFrameData
{
public:
    explicit CFrameData(const CRenderContext* context);
    CFrameData(const CFrameData&) = delete;
    CFrameData(CFrameData&&) noexcept = default;
    CFrameData& operator=(const CFrameData&) = delete;
    CFrameData& operator=(CFrameData&&) noexcept = default;
    ~CFrameData() = default;

    [[nodiscard]] const vk::raii::CommandPool& GetCommandPool() const { return m_commandPool; }
    [[nodiscard]] const vk::raii::CommandBuffers& GetCommandBuffers() const { return m_commandBuffer; }
    [[nodiscard]] const vk::raii::Fence& GetFence() const { return m_fence; }
    [[nodiscard]] const vk::raii::Semaphore& GetImageAvailableSemaphore() const { return m_imageAvailableSemaphore; }

private:
    vk::raii::CommandPool m_commandPool = VK_NULL_HANDLE;
    vk::raii::CommandBuffers m_commandBuffer = VK_NULL_HANDLE;
    vk::raii::Fence m_fence = VK_NULL_HANDLE;
    vk::raii::Semaphore m_imageAvailableSemaphore = VK_NULL_HANDLE;

    const CRenderContext* m_context;
};
}
