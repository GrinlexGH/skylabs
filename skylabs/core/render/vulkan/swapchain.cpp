#include <skylabs/core/render/vulkan/swapchain.hpp>
#include <skylabs/public/logging.hpp>

namespace Vulkan {
CSwapchain::CSwapchain(
    const vk::raii::PhysicalDevice& physicalDevice,
    const CDevice& device,
    const IWindow* window,
    const vk::raii::SurfaceKHR& surface,
    const std::uint32_t imageCount,
    const vk::PresentModeKHR presentMode
) : m_physicalDevice(&physicalDevice),
    m_device(&device),
    m_window(window),
    m_surface(&surface)
{
    CreateSwapchain(surface, imageCount, presentMode);
}

void CSwapchain::Recreate(const SwapchainRecreateInfo& recreateInfo) {
    CreateSwapchain(
        *m_surface,
        recreateInfo.m_imageCount.value_or(m_images.size()),
        recreateInfo.m_presentMode.value_or(m_presentMode),
        *m_handle
    );
}

void CSwapchain::CreateSwapchain(
    const vk::SurfaceKHR& surface,
    const std::uint32_t imageCount,
    const vk::PresentModeKHR presentMode,
    VkSwapchainKHR oldHandle
) {
    assert(m_device->IsExtensionEnabled(vk::KHRSwapchainExtensionName));

    const vk::SurfaceCapabilitiesKHR caps = m_physicalDevice->getSurfaceCapabilitiesKHR(surface);
    m_surfaceTransform = caps.currentTransform;

    const auto [width, height] = m_window->DrawableSize();

    vkb::SwapchainBuilder builder { m_device->VkbDevice(), surface };
    auto swapchainResult = builder
        .set_old_swapchain(oldHandle)
        .use_default_format_selection()
        .set_desired_present_mode(static_cast<VkPresentModeKHR>(presentMode))
        .add_fallback_present_mode(static_cast<VkPresentModeKHR>(vk::PresentModeKHR::eMailbox))
        .add_fallback_present_mode(static_cast<VkPresentModeKHR>(vk::PresentModeKHR::eFifo))
        .use_default_image_usage_flags()
        .add_fallback_format(static_cast<VkSurfaceFormatKHR>(
            vk::SurfaceFormatKHR { vk::Format::eR8G8B8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear }
        ))
        .set_desired_extent(width, height)
        .set_desired_min_image_count(imageCount)
        .set_pre_transform_flags(static_cast<VkSurfaceTransformFlagBitsKHR>(m_surfaceTransform))
        .build();

    if (!swapchainResult) {
        throw std::runtime_error(
            fmt::format("Failed to create vulkan device ({}): {}, {}",
                vk::to_string(vk::Result { swapchainResult.vk_result() }),
                swapchainResult.error().message(),
                fmt::join(swapchainResult.detailed_failure_reasons(), "; ")
            )
        );
    }

    vkb::Swapchain sw = swapchainResult.value();
    m_handle = vk::raii::SwapchainKHR { **m_device, sw.swapchain };
    m_extent = sw.extent;
    m_presentMode = static_cast<vk::PresentModeKHR>(sw.present_mode);
    m_surfaceFormat = { .format = sw.image_format, .colorSpace = sw.color_space };

    Log::Debug(
        "Swapchain format: {} x {}",
        vk::to_string(static_cast<vk::Format>(sw.image_format)),
        vk::to_string(static_cast<vk::ColorSpaceKHR>(sw.color_space))
    );

    CreateImages();
}

void CSwapchain::CreateImages() {
    m_images.clear();
    for (auto& image : m_handle.getImages()) {
        m_images.emplace_back(**m_device, image,
            vk::Extent3D { m_extent, 1 }, m_surfaceFormat.format,
            1, 1,
            vk::SampleCountFlagBits::e1
        );
    }
}

void CSwapchain::Clear() {
    m_handle.clear();
    m_images.clear();
}

std::pair<vk::Result, std::uint32_t> CSwapchain::AcquireImage(const vk::Semaphore& semaphore, const vk::Fence& fence) const {
    std::uint32_t imageIndex = 0;
    const auto result = static_cast<vk::Result>(m_handle.getDispatcher()->vkAcquireNextImageKHR(
        static_cast<VkDevice>(m_handle.getDevice()),
        static_cast<VkSwapchainKHR>(*m_handle),
        UINT64_MAX,
        static_cast<VkSemaphore>(semaphore),
        static_cast<VkFence>(fence),
        &imageIndex
    ));

    return { result, imageIndex };
}

vk::Result CSwapchain::PresentImage(std::uint32_t imageIndex, const vk::ArrayProxy<const vk::Semaphore>& semaphores) const {
    vk::PresentInfoKHR presentInfo {};
    presentInfo.setWaitSemaphores(semaphores);
    presentInfo.setSwapchains({ *m_handle });
    presentInfo.setImageIndices({ imageIndex });

    const vk::raii::Queue& queue = *m_device->PresentQueue();
    const auto result = static_cast<vk::Result>(queue.getDispatcher()->vkQueuePresentKHR(
        static_cast<VkQueue>(*queue),
        &static_cast<const VkPresentInfoKHR&>(presentInfo)
    ));

    return result;
}
}
