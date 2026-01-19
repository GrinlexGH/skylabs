#pragma once
#include <skylabs/core/render/vulkan/context/context.hpp>

namespace Vulkan {
class CSwapchain
{
public:
    explicit CSwapchain(std::nullptr_t) {}
    explicit CSwapchain(
        const CContext& context,
        const vk::SurfaceKHR& surface,
        std::uint32_t imageCount,
        vk::PresentModeKHR presentMode
    );
    CSwapchain(const CSwapchain&) = delete;
    CSwapchain(CSwapchain&&) noexcept = default;
    CSwapchain& operator=(const CSwapchain&) = delete;
    CSwapchain& operator=(CSwapchain&&) noexcept = default;
    ~CSwapchain() = default;

    [[nodiscard]] const vk::raii::SwapchainKHR& operator*() const noexcept { return m_handle; }
    [[nodiscard]] const vk::raii::SwapchainKHR* operator->() const noexcept { return &m_handle; }

    [[nodiscard]] vk::SurfaceFormatKHR SurfaceFormat() const { return m_surfaceFormat; }
    [[nodiscard]] vk::Extent2D Extent() const { return m_extent; }
    [[nodiscard]] vk::PresentModeKHR PresentMode() const { return m_presentMode; }
    [[nodiscard]] vk::SurfaceKHR Surface() const { return m_associatedSurface; }

    [[nodiscard]] const std::vector<vk::Image>& Images() const { return m_images; }
    [[nodiscard]] const std::vector<vk::raii::ImageView>& ImageViews() const { return m_imageViews; }
    [[nodiscard]] std::uint32_t ImageCount() const {
        assert(m_images.size() == m_imageViews.size());
        return static_cast<std::uint32_t>(m_images.size());
    }

    void Recreate();
    void Recreate(const vk::SurfaceKHR& surface, std::uint32_t imageCount, vk::PresentModeKHR presentMode);

private:
    void CreateSwapchain(
        const vk::SurfaceKHR& surface,
        std::uint32_t imageCount,
        vk::PresentModeKHR presentMode,
        const vk::raii::SwapchainKHR& oldSwapchain = nullptr
    );
    void CreateImages();
    void DestroyImages();

    [[nodiscard]] vk::Extent2D ChooseSurfaceExtent(const vk::SurfaceCapabilitiesKHR& capabilities) const;

    vk::raii::SwapchainKHR m_handle = nullptr;

    vk::SurfaceFormatKHR m_surfaceFormat;
    vk::Extent2D m_extent;
    vk::PresentModeKHR m_presentMode = vk::PresentModeKHR::eImmediate;
    vk::SurfaceKHR m_associatedSurface;

    std::vector<vk::Image> m_images;
    std::vector<vk::raii::ImageView> m_imageViews;

    const CContext* m_context = nullptr;
};
}
