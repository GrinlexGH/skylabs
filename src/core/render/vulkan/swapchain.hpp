#pragma once
#include "vulkan.hpp"
#include "render_context.hpp"

namespace Vulkan {
class CSwapchain
{
public:
    struct CInfo
    {
        vk::SurfaceFormatKHR m_format;
        vk::Extent2D m_extent;
    };

    explicit CSwapchain(
        const CRenderContext* context,
        const vk::SurfaceKHR& surface,
        std::uint32_t imageCount,
        vk::PresentModeKHR presentMode,
        const vk::SwapchainKHR& oldSwaphchain = nullptr
    );
    CSwapchain(const CSwapchain&) = delete;
    CSwapchain(CSwapchain&&) = delete;
    CSwapchain& operator=(const CSwapchain&) = delete;
    CSwapchain& operator=(CSwapchain&&) = delete;
    ~CSwapchain();

    [[nodiscard]] vk::SwapchainKHR GetHandle() const { return m_handle; }
    [[nodiscard]] const CInfo& GetInfo() const { return m_info; }
    [[nodiscard]] const std::vector<vk::Image>& GetImages() const { return m_images; }
    [[nodiscard]] const std::vector<vk::ImageView>& GetImageViews() const { return m_imageViews; }

private:
    [[nodiscard]] vk::Extent2D ChooseSurfaceExtent(const vk::SurfaceCapabilitiesKHR& capabilities) const;

    vk::SwapchainKHR m_handle = VK_NULL_HANDLE;

    CInfo m_info;
    std::vector<vk::Image> m_images {};
    std::vector<vk::ImageView> m_imageViews {};

    const CRenderContext* m_context;
};
}
