#include <skylabs/core/render/vulkan/platform/swapchain.hpp>
#include <skylabs/public/logging.hpp>

#include <fmt/ranges.h>

namespace Vulkan {
CSwapchain::CSwapchain(
    const CContext& context,
    const vk::SurfaceKHR surface,
    const std::uint32_t imageCount,
    const vk::PresentModeKHR presentMode
) : m_context(&context), m_associatedSurface(surface) {
    CreateSwapchain(surface, imageCount, presentMode);
}

CSwapchain::CSwapchain(
    CSwapchain&& oldSwapchain,
    const std::uint32_t imageCount,
    const vk::PresentModeKHR presentMode
) : m_context(oldSwapchain.m_context), m_associatedSurface(oldSwapchain.m_associatedSurface) {
    CreateSwapchain(m_associatedSurface, imageCount, presentMode, *oldSwapchain);
}

void CSwapchain::CreateSwapchain(
    const vk::SurfaceKHR surface,
    const std::uint32_t imageCount,
    vk::PresentModeKHR presentMode,
    const vk::raii::SwapchainKHR& oldSwapchain
) {
    const CDevice& device = m_context->Device();
    assert(device.IsExtensionEnabled(vk::KHRSwapchainExtensionName));

    vkb::SwapchainBuilder builder { device.VkbDevice(), surface };
    auto swapchainResult = builder
        .set_old_swapchain(*oldSwapchain)
        .use_default_format_selection()
        .set_desired_present_mode(static_cast<VkPresentModeKHR>(presentMode))
        .add_fallback_present_mode(static_cast<VkPresentModeKHR>(vk::PresentModeKHR::eMailbox))
        .add_fallback_present_mode(static_cast<VkPresentModeKHR>(vk::PresentModeKHR::eFifo))
        .use_default_image_usage_flags()
        .set_desired_min_image_count(imageCount)
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

    auto sw = swapchainResult.value();
    m_handle = vk::raii::SwapchainKHR { *device, sw.swapchain };
    m_extent = sw.extent;
    m_presentMode = static_cast<vk::PresentModeKHR>(sw.present_mode);
    m_surfaceFormat = { sw.image_format, sw.color_space };

    CreateImages();
}

void CSwapchain::CreateImages() {
    m_images.clear();
    for (auto& image : m_handle.getImages()) {
        m_images.emplace_back(*m_context, image, vk::Extent3D { m_extent, 1 }, m_surfaceFormat.format, 1, 1, vk::SampleCountFlagBits::e1);
    }
}

std::expected<std::uint32_t, vk::Result> CSwapchain::AcquireImage(vk::Semaphore semaphore, vk::Fence fence) const {
    auto [result, index] = m_handle.acquireNextImage(UINT64_MAX, semaphore, fence);

    if (result == vk::Result::eSuccess) {
        return index;
    } else if (result == vk::Result::eSuboptimalKHR || result == vk::Result::eErrorOutOfDateKHR) {
        return std::unexpected(result);
    }

    throw std::runtime_error("Failed to acquire swapchain image: " + vk::to_string(result));
}

vk::Result CSwapchain::PresentImage(std::uint32_t imageIndex, const vk::ArrayProxyNoTemporaries<const vk::Semaphore>& semaphores) const {
    vk::PresentInfoKHR presentInfo {};
    presentInfo.setWaitSemaphores(semaphores);
    presentInfo.setSwapchains({ *m_handle });
    presentInfo.setImageIndices({ imageIndex });
    return m_context->Device().GraphicsQueue()->presentKHR(presentInfo);
}
}
