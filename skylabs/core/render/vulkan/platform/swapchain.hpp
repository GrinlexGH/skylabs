#pragma once
#include <skylabs/core/render/vulkan/context/context.hpp>
#include <skylabs/core/render/vulkan/resources/image.hpp>

namespace Vulkan {
class CSwapchain
{
public:
    explicit CSwapchain(std::nullptr_t) {}
    explicit CSwapchain(
        const CContext& context,
        vk::SurfaceKHR surface,
        std::uint32_t imageCount,
        vk::PresentModeKHR presentMode
    );
    explicit CSwapchain(
        CSwapchain&& oldSwapchain,
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

    [[nodiscard]] const std::vector<CImage>& Images() const { return m_images; }

private:
    void CreateSwapchain(
        vk::SurfaceKHR surface,
        std::uint32_t imageCount,
        vk::PresentModeKHR presentMode,
        const vk::raii::SwapchainKHR& oldSwapchain = nullptr
    );
    void CreateImages();

    [[nodiscard]] vk::Extent2D ChooseSurfaceExtent(const vk::SurfaceCapabilitiesKHR& capabilities) const;
    [[nodiscard]] std::uint32_t ChooseImageCount(const vk::SurfaceCapabilitiesKHR& capabilities, std::uint32_t requestedCount) const;
    [[nodiscard]] vk::PresentModeKHR ChoosePresentMode(vk::PresentModeKHR requestedMode) const;

    const CContext* m_context = nullptr;

    vk::raii::SwapchainKHR m_handle = nullptr;

    vk::SurfaceFormatKHR m_surfaceFormat;
    vk::SurfaceKHR m_associatedSurface;
    vk::Extent2D m_extent;
    vk::PresentModeKHR m_presentMode = vk::PresentModeKHR::eFifo;

    std::vector<CImage> m_images;
};
}
