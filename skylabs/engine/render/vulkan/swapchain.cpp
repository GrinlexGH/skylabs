#include <VkBootstrap.h>
#include <fmt/ranges.h>

#include "skylabs/engine/logging.hpp"
#include "skylabs/engine/render/vulkan/swapchain.hpp"

namespace sk::render::vulkan {
Swapchain::Swapchain(const Device& device, const IWindow* window, const vk::raii::SurfaceKHR& surface,
                     const std::uint32_t imageCount, const vk::PresentModeKHR presentMode,
                     const VkSwapchainKHR oldHandle)
    : m_device(&device), m_window(window), m_surface(&surface) {
    assert(device.IsExtensionEnabled(vk::KHRSwapchainExtensionName));

    const vk::SurfaceCapabilities2KHR caps = SurfaceCapabilities();
    m_extent = SurfaceExtent();

    vkb::SwapchainBuilder builder { **device.GetPhysicalDevice(), **device, *surface,
                                    device.GraphicsQueue().FamilyIndex(),
                                    device.PresentQueue().FamilyIndex() };
    auto swapchainResult =
        builder.set_old_swapchain(oldHandle)
            .use_default_format_selection()
            .set_desired_present_mode(static_cast<VkPresentModeKHR>(presentMode))
            .add_fallback_present_mode(static_cast<VkPresentModeKHR>(vk::PresentModeKHR::eMailbox))
            .add_fallback_present_mode(static_cast<VkPresentModeKHR>(vk::PresentModeKHR::eFifo))
            .use_default_image_usage_flags()
            .add_fallback_format(
                vk::SurfaceFormatKHR { vk::Format::eR8G8B8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear })
            .set_desired_extent(m_extent.width, m_extent.height)
            .set_desired_min_image_count(imageCount)
            .set_pre_transform_flags(
                static_cast<VkSurfaceTransformFlagBitsKHR>(caps.surfaceCapabilities.currentTransform))
            .build();

    if (!swapchainResult) {
        throw std::runtime_error(fmt::format(
            "Failed to create vulkan device ({}): {}, {}",
            vk::to_string(vk::Result { swapchainResult.vk_result() }), swapchainResult.error().message(),
            fmt::join(swapchainResult.detailed_failure_reasons(), "; ")));
    }

    vkb::Swapchain sw = swapchainResult.value();
    m_handle = vk::raii::SwapchainKHR { *device, sw.swapchain };
    m_extent = sw.extent;
    m_presentMode = static_cast<vk::PresentModeKHR>(sw.present_mode);
    m_surfaceFormat = { .format = sw.image_format, .colorSpace = sw.color_space };

    log::Debug(
        "Swapchain created: {} x {} ({}x{})", vk::to_string(static_cast<vk::Format>(sw.image_format)),
        vk::to_string(static_cast<vk::ColorSpaceKHR>(sw.color_space)), m_extent.width, m_extent.height);

    const std::vector<vk::Image> images = m_handle.getImages();
    m_images.reserve(images.size());
    m_renderFinishedSemaphores.reserve(images.size());
    for (auto& image : images) {
        m_images.emplace_back(*device, image, vk::Extent3D { m_extent, 1 }, m_surfaceFormat.format, 1, 1,
                              vk::SampleCountFlagBits::e1, vk::ImageViewType::e2D);
        m_renderFinishedSemaphores.emplace_back(*device, vk::SemaphoreCreateInfo { });
    }
}

Swapchain::Swapchain(const Swapchain& oldSwapchain, const SwapchainRecreateInfo& recreationInfo)
    : Swapchain(*oldSwapchain.m_device, oldSwapchain.m_window, *oldSwapchain.m_surface,
                recreationInfo.imageCount.value_or(oldSwapchain.m_images.size()),
                recreationInfo.presentMode.value_or(oldSwapchain.PresentMode()),
                *oldSwapchain.m_handle) { }

void Swapchain::Clear() {
    m_handle.clear();
    m_images.clear();
    m_renderFinishedSemaphores.clear();
}

vk::SurfaceCapabilities2KHR Swapchain::SurfaceCapabilities() const {
    const vk::PhysicalDeviceSurfaceInfo2KHR surfaceInfo { *m_surface };
    return m_device->GetPhysicalDevice()->getSurfaceCapabilities2KHR(surfaceInfo);
}

vk::Extent2D Swapchain::SurfaceExtent() const {
    const vk::SurfaceCapabilities2KHR caps = SurfaceCapabilities();

    vk::Extent2D surfaceExtent = caps.surfaceCapabilities.currentExtent;
    if (surfaceExtent.width == std::numeric_limits<std::uint32_t>::max()) {
        const auto [width, height] = m_window->DrawableSize();
        surfaceExtent = vk::Extent2D { width, height };
    }

    assert(surfaceExtent.height != 0 && surfaceExtent.width != 0);

    return surfaceExtent;
}

std::pair<vk::Result, std::uint32_t> Swapchain::AcquireImage(const vk::Semaphore& semaphore,
                                                             const vk::Fence& fence) const {
    std::uint32_t imageIndex = 0;
    const auto result = static_cast<vk::Result>(m_handle.getDispatcher()->vkAcquireNextImageKHR(
        static_cast<VkDevice>(m_handle.getDevice()), static_cast<VkSwapchainKHR>(*m_handle), UINT64_MAX,
        static_cast<VkSemaphore>(semaphore), static_cast<VkFence>(fence), &imageIndex));

    return { result, imageIndex };
}

vk::Result Swapchain::PresentImage(std::uint32_t imageIndex,
                                   const vk::ArrayProxy<const vk::Semaphore>& semaphores) const {
    vk::PresentInfoKHR presentInfo { };
    presentInfo.setWaitSemaphores(semaphores);
    presentInfo.setSwapchains({ *m_handle });
    presentInfo.setImageIndices({ imageIndex });

    const vk::raii::Queue& queue = *m_device->PresentQueue();
    const auto result = static_cast<vk::Result>(queue.getDispatcher()->vkQueuePresentKHR(
        static_cast<VkQueue>(*queue), &static_cast<const VkPresentInfoKHR&>(presentInfo)));

    return result;
}

Swapchain::Swapchain(const Device& device, const IWindow* window, const vk::raii::SurfaceKHR& surface,
                     vk::raii::SwapchainKHR&& handle, const vk::SurfaceFormatKHR surfaceFormat,
                     const vk::SurfaceTransformFlagBitsKHR transform, const vk::Extent2D extent,
                     const vk::PresentModeKHR presentMode, std::vector<Image>&& images,
                     std::vector<vk::raii::Semaphore>&& renderFinishedSemaphores)
    : m_device(&device),
      m_window(window),
      m_surface(&surface),
      m_handle(std::move(handle)),
      m_surfaceFormat(surfaceFormat),
      m_surfaceTransform(transform),
      m_extent(extent),
      m_presentMode(presentMode),
      m_images(std::move(images)),
      m_renderFinishedSemaphores(std::move(renderFinishedSemaphores)) { }
}
