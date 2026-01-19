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

    [[nodiscard]] const vk::raii::CommandPool& GetCommandPool() const { return m_commandPool; }
    [[nodiscard]] const vk::raii::CommandBuffers& GetCommandBuffers() const { return m_commandBuffer; }
    [[nodiscard]] const vk::raii::Fence& GetFence() const { return m_fence; }
    [[nodiscard]] const vk::raii::Semaphore& GetImageAvailableSemaphore() const { return m_imageAvailableSemaphore; }

    void RecreateImageAvailableSemaphore() { m_imageAvailableSemaphore = vk::raii::Semaphore { *m_context->Device(), vk::SemaphoreCreateInfo {} }; }

private:
    const CContext* m_context = nullptr;

    vk::raii::CommandPool m_commandPool = nullptr;
    vk::raii::CommandBuffers m_commandBuffer = nullptr;
    vk::raii::Fence m_fence = nullptr;
    vk::raii::Semaphore m_imageAvailableSemaphore = nullptr;
};
}
