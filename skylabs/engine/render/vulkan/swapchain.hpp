#pragma once
#include "skylabs/engine/render/vulkan/context.hpp"
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
    explicit Swapchain(const vk::raii::PhysicalDevice& physicalDevice, const Device& device,
                       const Queues& queues, const IWindow* window, const vk::raii::SurfaceKHR& surface,
                       std::uint32_t imageCount, vk::PresentModeKHR presentMode,
                       VkSwapchainKHR oldHandle = VK_NULL_HANDLE);
    explicit Swapchain(const Swapchain& oldSwapchain, const SwapchainRecreateInfo& recreationInfo);

    [[nodiscard]] const vk::raii::SwapchainKHR& operator*() const noexcept { return m_handle; }
    [[nodiscard]] const vk::raii::SwapchainKHR* operator->() const noexcept { return &m_handle; }

    void Clear();

    [[nodiscard]] bool IsOutdated() const;

    [[nodiscard]] vk::SurfaceFormatKHR SurfaceFormat() const { return m_surfaceFormat; }
    [[nodiscard]] vk::SurfaceTransformFlagsKHR SurfaceTransform() const { return m_surfaceTransform; }
    [[nodiscard]] vk::Extent2D Extent() const { return m_extent; }
    [[nodiscard]] vk::PresentModeKHR PresentMode() const { return m_presentMode; }

    [[nodiscard]] std::span<Attachment> Images() { return m_images; }
    [[nodiscard]] std::span<vk::raii::Semaphore> RenderFinishedSemaphores() {
        return m_renderFinishedSemaphores;
    }

private:
    struct InitData;
    static InitData Initialize(const vk::raii::PhysicalDevice& physicalDevice, const Device& device,
                               const Queues& queues, const IWindow* window,
                               const vk::raii::SurfaceKHR& surface, std::uint32_t imageCount,
                               vk::PresentModeKHR presentMode, VkSwapchainKHR oldHandle);
    Swapchain(InitData&& initData);

    const vk::raii::PhysicalDevice* m_physicalDevice;
    const Device* m_device;
    const Queues* m_queues;
    const IWindow* m_window;
    const vk::raii::SurfaceKHR* m_surface;

    vk::raii::SwapchainKHR m_handle { nullptr };

    vk::SurfaceFormatKHR m_surfaceFormat;
    vk::SurfaceTransformFlagsKHR m_surfaceTransform;
    vk::Extent2D m_extent;
    vk::PresentModeKHR m_presentMode;

    std::vector<Attachment> m_images;
    std::vector<vk::raii::Semaphore> m_renderFinishedSemaphores;
};
}
