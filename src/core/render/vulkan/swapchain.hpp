#pragma once
#include "context/context.hpp"

namespace Vulkan {
class CSwapchain
{
public:
    explicit CSwapchain(std::nullptr_t);
    explicit CSwapchain(
        const CContext* context,
        const vk::SurfaceKHR& surface,
        std::uint32_t imageCount,
        vk::PresentModeKHR presentMode
    );
    CSwapchain(const CSwapchain&) = delete;
    CSwapchain(CSwapchain&&) noexcept = default;
    CSwapchain& operator=(const CSwapchain&) = delete;
    CSwapchain& operator=(CSwapchain&&) noexcept = default;
    ~CSwapchain() = default;

    [[nodiscard]] auto GetHandle() const -> const vk::raii::SwapchainKHR& { return m_handle; }

    [[nodiscard]] auto GetSurfaceFormat() const -> vk::SurfaceFormatKHR { return m_surfaceFormat; }
    [[nodiscard]] auto GetExtent() const -> vk::Extent2D { return m_extent; }
    [[nodiscard]] auto GetImageCount() const -> std::uint32_t { return m_imageCount; }
    [[nodiscard]] auto GetPresentMode() const -> vk::PresentModeKHR { return m_presentMode; }
    [[nodiscard]] auto GetSurface() const -> vk::SurfaceKHR { return m_associatedSurface; }

    [[nodiscard]] auto GetImages() const -> const std::vector<vk::Image>& { return m_images; }
    [[nodiscard]] auto GetImageViews() const -> const std::vector<vk::raii::ImageView>& { return m_imageViews; }

    auto Recreate() -> void;
    auto Recreate(const vk::SurfaceKHR& surface, std::uint32_t imageCount, vk::PresentModeKHR presentMode) -> void;

private:
    auto CreateSwapchain(
        const vk::SurfaceKHR& surface,
        std::uint32_t imageCount,
        vk::PresentModeKHR presentMode,
        const vk::raii::SwapchainKHR& oldSwapchain = nullptr
    ) -> void;
    auto CreateImages() -> void;
    auto DestroyImages() -> void;

    [[nodiscard]] auto ChooseSurfaceExtent(const vk::SurfaceCapabilitiesKHR& capabilities) const -> vk::Extent2D;

    vk::raii::SwapchainKHR m_handle = nullptr;

    vk::SurfaceFormatKHR m_surfaceFormat;
    vk::Extent2D m_extent;
    std::uint32_t m_imageCount;
    vk::PresentModeKHR m_presentMode;
    vk::SurfaceKHR m_associatedSurface;

    std::vector<vk::Image> m_images;
    std::vector<vk::raii::ImageView> m_imageViews;

    const CContext* m_context;
};
}
