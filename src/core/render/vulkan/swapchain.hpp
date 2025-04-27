#pragma once
#include "vulkan.hpp"
#include "render_context.hpp"

namespace Vulkan {
class CSwapchain
{
public:
    explicit CSwapchain(
        const CRenderContext* context,
        const vk::SurfaceKHR& surface,
        std::uint32_t imageCount,
        const vk::PresentModeKHR& vSync,
        const vk::SwapchainKHR& oldSwaphchain = nullptr
    );
    CSwapchain(const CSwapchain&) = delete;
    CSwapchain(CSwapchain&&) = delete;
    CSwapchain& operator=(const CSwapchain&) = delete;
    CSwapchain& operator=(CSwapchain&&) = delete;
    ~CSwapchain();

    [[nodiscard]] vk::SwapchainKHR GetHandle() const { return m_handle; }

private:
    vk::SwapchainKHR m_handle = VK_NULL_HANDLE;

    const CRenderContext* m_context;
};
}
