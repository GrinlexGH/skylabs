#pragma once
#include "skylabs/engine/render/vulkan/context/device.hpp"
#include "skylabs/engine/render/vulkan/image.hpp"
#include "skylabs/engine/window.hpp"

namespace sk::render::vulkan {
struct SwapchainRecreateInfo {
    std::optional<std::uint32_t> imageCount = std::nullopt;
    std::optional<vk::PresentModeKHR> presentMode = std::nullopt;
};

class Swapchain {
public:
    explicit Swapchain(std::nullptr_t) { }
    explicit Swapchain(const Device& device, const IWindow* window, const vk::raii::SurfaceKHR& surface,
                       std::uint32_t imageCount, vk::PresentModeKHR presentMode,
                       VkSwapchainKHR oldHandle = VK_NULL_HANDLE);
    explicit Swapchain(const Swapchain& oldSwapchain, const SwapchainRecreateInfo& recreationInfo);
    Swapchain(const Swapchain&) = delete;
    Swapchain(Swapchain&&) noexcept = default;
    Swapchain& operator=(const Swapchain&) = delete;
    Swapchain& operator=(Swapchain&&) noexcept = default;
    ~Swapchain() = default;

    [[nodiscard]] const vk::raii::SwapchainKHR& operator*() const noexcept { return m_handle; }
    [[nodiscard]] const vk::raii::SwapchainKHR* operator->() const noexcept { return &m_handle; }

    void Clear();

    [[nodiscard]] std::pair<vk::Result, std::uint32_t> AcquireImage(const vk::Semaphore& semaphore = { },
                                                                    const vk::Fence& fence = { }) const;
    [[nodiscard]] vk::Result PresentImage(
        std::uint32_t imageIndex, const vk::ArrayProxy<const vk::Semaphore>& semaphores = { }) const;

    [[nodiscard]] vk::SurfaceFormatKHR SurfaceFormat() const { return m_surfaceFormat; }
    [[nodiscard]] vk::SurfaceTransformFlagBitsKHR SurfaceTransform() const { return m_surfaceTransform; }
    [[nodiscard]] vk::Extent2D Extent() const { return m_extent; }
    [[nodiscard]] vk::PresentModeKHR PresentMode() const { return m_presentMode; }

    [[nodiscard]] std::span<Image> Images() { return m_images; }
    [[nodiscard]] std::span<vk::raii::Semaphore> RenderFinishedSemaphores() {
        return m_renderFinishedSemaphores;
    }

private:
    explicit Swapchain(const Device& device, const IWindow* window, const vk::raii::SurfaceKHR& surface,
                       vk::raii::SwapchainKHR&& handle, vk::SurfaceFormatKHR surfaceFormat,
                       vk::SurfaceTransformFlagBitsKHR transform, vk::Extent2D extent,
                       vk::PresentModeKHR presentMode, std::vector<Image>&& images,
                       std::vector<vk::raii::Semaphore>&& renderFinishedSemaphores);

    const Device* m_device = nullptr;
    const IWindow* m_window = nullptr;
    const vk::raii::SurfaceKHR* m_surface = nullptr;

    vk::raii::SwapchainKHR m_handle = nullptr;

    vk::SurfaceFormatKHR m_surfaceFormat;
    vk::SurfaceTransformFlagBitsKHR m_surfaceTransform = vk::SurfaceTransformFlagBitsKHR::eIdentity;
    vk::Extent2D m_extent;
    vk::PresentModeKHR m_presentMode = vk::PresentModeKHR::eFifo;

    std::vector<Image> m_images;
    std::vector<vk::raii::Semaphore> m_renderFinishedSemaphores;
};
}
