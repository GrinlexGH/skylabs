#pragma once
#include <skylabs/core/render/vulkan/context/context.hpp>

namespace Vulkan {
class CFrameData
{
public:
    explicit CFrameData(const CContext& context);
    CFrameData(const CFrameData&) = delete;
    CFrameData(CFrameData&&) noexcept = default;
    CFrameData& operator=(const CFrameData&) = delete;
    CFrameData& operator=(CFrameData&&) noexcept = default;
    ~CFrameData() = default;

    [[nodiscard]] auto GetCommandPool() const -> const vk::raii::CommandPool& { return m_commandPool; }
    [[nodiscard]] auto GetCommandBuffers() const -> const vk::raii::CommandBuffers& { return m_commandBuffer; }
    [[nodiscard]] auto GetFence() const -> const vk::raii::Fence& { return m_fence; }
    [[nodiscard]] auto GetImageAvailableSemaphore() const -> const vk::raii::Semaphore& { return m_imageAvailableSemaphore; }

private:
    vk::raii::CommandPool m_commandPool = nullptr;
    vk::raii::CommandBuffers m_commandBuffer = nullptr;
    vk::raii::Fence m_fence = nullptr;
    vk::raii::Semaphore m_imageAvailableSemaphore = nullptr;
};
}
