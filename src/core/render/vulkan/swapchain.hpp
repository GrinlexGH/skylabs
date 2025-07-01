#pragma once
#include "render_context.hpp"

#include <vulkan/vulkan.hpp>

namespace Vulkan {
class CSwapchain
{
public:
    struct CInfo
    {
        vk::SurfaceFormatKHR m_surfaceFormat;
        vk::Extent2D m_extent;
        std::uint32_t m_imageCount;
        vk::PresentModeKHR m_presentMode;
        vk::SurfaceKHR m_associatedSurface;
    };

    explicit CSwapchain(
        const CRenderContext* context,
        const vk::SurfaceKHR& surface,
        std::uint32_t imageCount,
        vk::PresentModeKHR presentMode
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

    void Recreate();
    void Recreate(const vk::SurfaceKHR& surface, std::uint32_t imageCount, vk::PresentModeKHR presentMode);

private:
    void CreateSwapchain(
        const vk::SurfaceKHR& surface,
        std::uint32_t imageCount,
        vk::PresentModeKHR presentMode,
        const vk::SwapchainKHR& oldSwapchain = VK_NULL_HANDLE
    );
    void CreateImages();
    void DestroyImages();

    [[nodiscard]] vk::Extent2D ChooseSurfaceExtent(const vk::SurfaceCapabilitiesKHR& capabilities) const;

    vk::SwapchainKHR m_handle = VK_NULL_HANDLE;

    CInfo m_info;
    std::vector<vk::Image> m_images;
    std::vector<vk::ImageView> m_imageViews;

    const CRenderContext* m_context;
};
}
