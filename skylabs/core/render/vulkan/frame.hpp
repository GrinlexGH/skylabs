#pragma once
#include <skylabs/core/render/vulkan/context/context.hpp>
#include <skylabs/core/render/vulkan/command_buffer_set.hpp>

namespace Vulkan {
class CFrame
{
public:
    explicit CFrame(std::nullptr_t) {}
    explicit CFrame(const CContext& context);
    CFrame(const CFrame&) = delete;
    CFrame(CFrame&&) noexcept = default;
    CFrame& operator=(const CFrame&) = delete;
    CFrame& operator=(CFrame&&) noexcept = default;
    ~CFrame() = default;

    [[nodiscard]] const vk::raii::Fence& GetFence() const { return m_fence; }
    [[nodiscard]] const vk::raii::Semaphore& GetImageAvailableSemaphore() const { return m_imageAvailableSemaphore; }

    void RecreateImageAvailableSemaphore() { m_imageAvailableSemaphore = vk::raii::Semaphore { *m_context->Device(), vk::SemaphoreCreateInfo {} }; }

private:
    const CContext* m_context = nullptr;

    vk::raii::Fence m_fence = nullptr;
    vk::raii::Semaphore m_imageAvailableSemaphore = nullptr;
};
}
