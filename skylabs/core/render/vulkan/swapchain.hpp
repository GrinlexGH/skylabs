#pragma once
#include <skylabs/core/render/vulkan/context/device.hpp>
#include <skylabs/core/render/vulkan/resources/image.hpp>
#include <skylabs/public/window.hpp>

namespace Vulkan {
struct SwapchainRecreateInfo
{
    std::optional<std::uint32_t> m_imageCount = std::nullopt;
    std::optional<vk::PresentModeKHR> m_presentMode = std::nullopt;
};

class CSwapchain
{
public:
    explicit CSwapchain(std::nullptr_t) {}
    explicit CSwapchain(
        const vk::raii::PhysicalDevice& physicalDevice,
        const CDevice& device,
        const IWindow* window,
        const vk::raii::SurfaceKHR& surface,
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

    void Recreate(const SwapchainRecreateInfo& recreateInfo);
    void Clear();

    [[nodiscard]] std::pair<vk::Result, std::uint32_t> AcquireImage(const vk::Semaphore& semaphore = {}, const vk::Fence& fence = {}) const;
    [[nodiscard]] vk::Result PresentImage(std::uint32_t imageIndex, const vk::ArrayProxy<const vk::Semaphore>& semaphores = {}) const;

    [[nodiscard]] vk::SurfaceFormatKHR SurfaceFormat() const { return m_surfaceFormat; }
    [[nodiscard]] vk::SurfaceTransformFlagBitsKHR SurfaceTransform() const { return m_surfaceTransform; }
    [[nodiscard]] vk::Extent2D Extent() const { return m_extent; }
    [[nodiscard]] vk::PresentModeKHR PresentMode() const { return m_presentMode; }

    [[nodiscard]] std::span<CImage> Images() { return m_images; }

private:
    void CreateSwapchain(
        const vk::SurfaceKHR& surface,
        std::uint32_t imageCount,
        vk::PresentModeKHR presentMode,
        VkSwapchainKHR oldHandle = nullptr
    );
    void CreateImages();

    const vk::raii::PhysicalDevice* m_physicalDevice = nullptr;
    const CDevice* m_device = nullptr;
    const IWindow* m_window = nullptr;
    const vk::raii::SurfaceKHR* m_surface = nullptr;

    vk::raii::SwapchainKHR m_handle = nullptr;

    vk::SurfaceFormatKHR m_surfaceFormat;
    vk::SurfaceTransformFlagBitsKHR m_surfaceTransform = vk::SurfaceTransformFlagBitsKHR::eIdentity;
    vk::Extent2D m_extent;
    vk::PresentModeKHR m_presentMode = vk::PresentModeKHR::eFifo;

    std::vector<CImage> m_images;
};
}
