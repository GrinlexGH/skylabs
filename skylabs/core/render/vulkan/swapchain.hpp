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

    [[nodiscard]] auto operator*() const noexcept -> const vk::raii::SwapchainKHR& { return m_handle; }
    [[nodiscard]] auto operator->() const noexcept -> const vk::raii::SwapchainKHR* { return &m_handle; }

    [[nodiscard]] auto SurfaceFormat() const -> vk::SurfaceFormatKHR { return m_surfaceFormat; }
    [[nodiscard]] auto Extent() const -> vk::Extent2D { return m_extent; }
    [[nodiscard]] auto PresentMode() const -> vk::PresentModeKHR { return m_presentMode; }
    [[nodiscard]] auto Surface() const -> vk::SurfaceKHR { return m_associatedSurface; }

    [[nodiscard]] auto Images() const -> const std::vector<vk::Image>& { return m_images; }
    [[nodiscard]] auto ImageViews() const -> const std::vector<vk::raii::ImageView>& { return m_imageViews; }
    [[nodiscard]] auto ImageCount() const -> std::uint32_t {
        assert(m_images.size() == m_imageViews.size());
        return static_cast<std::uint32_t>(m_images.size());
    }

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
    vk::PresentModeKHR m_presentMode = vk::PresentModeKHR::eImmediate;
    vk::SurfaceKHR m_associatedSurface;

    std::vector<vk::Image> m_images;
    std::vector<vk::raii::ImageView> m_imageViews;

    const CContext* m_context = nullptr;
};
}
