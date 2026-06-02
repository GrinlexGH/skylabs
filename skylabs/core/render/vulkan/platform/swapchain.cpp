#include <skylabs/core/render/vulkan/platform/swapchain.hpp>
#include <skylabs/public/logging.hpp>
#include <fmt/ranges.h>

namespace Vulkan {
CSwapchain::CSwapchain(
    const CDeviceContext& context,
    const std::uint32_t imageCount,
    const vk::PresentModeKHR presentMode
) : m_deviceContext(&context){
    CreateSwapchain(*context.Surface(), imageCount, presentMode);
}

void CSwapchain::Recreate(const SwapchainRecreateInfo& recreateInfo) {
    CreateSwapchain(
        *m_deviceContext->Surface(),
        recreateInfo.m_imageCount.value_or(m_images.size()),
        recreateInfo.m_presentMode.value_or(m_presentMode),
        *m_handle
    );
}

void CSwapchain::CreateSwapchain(
    const vk::SurfaceKHR surface,
    const std::uint32_t imageCount,
    const vk::PresentModeKHR presentMode,
    VkSwapchainKHR oldHandle
) {
    const CDevice& device = m_deviceContext->Device();
    assert(device.IsExtensionEnabled(vk::KHRSwapchainExtensionName));

    const vk::SurfaceCapabilitiesKHR caps = m_deviceContext->PhysicalDevice()->getSurfaceCapabilitiesKHR(surface);
    m_surfaceTransform = caps.currentTransform;

    const auto [width, height] = m_deviceContext->Window()->DrawableSize();

    vkb::SwapchainBuilder builder { device.VkbDevice(), surface };
    auto swapchainResult = builder
        .set_old_swapchain(oldHandle)
        .use_default_format_selection()
        .set_desired_present_mode(static_cast<VkPresentModeKHR>(presentMode))
        .add_fallback_present_mode(static_cast<VkPresentModeKHR>(vk::PresentModeKHR::eMailbox))
        .add_fallback_present_mode(static_cast<VkPresentModeKHR>(vk::PresentModeKHR::eFifo))
        .use_default_image_usage_flags()
        .set_desired_format(static_cast<VkSurfaceFormatKHR>(
            vk::SurfaceFormatKHR { vk::Format::eR16G16B16A16Sfloat, vk::ColorSpaceKHR::eHdr10St2084EXT }
        ))
        .add_fallback_format(static_cast<VkSurfaceFormatKHR>(
            vk::SurfaceFormatKHR { vk::Format::eA2R10G10B10SnormPack32, vk::ColorSpaceKHR::eHdr10St2084EXT }
        ))
        .add_fallback_format(static_cast<VkSurfaceFormatKHR>(
            vk::SurfaceFormatKHR { vk::Format::eR8G8B8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear }
        ))
        .add_fallback_format(static_cast<VkSurfaceFormatKHR>(
            vk::SurfaceFormatKHR { vk::Format::eB8G8R8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear }
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

    auto sw = swapchainResult.value();
    m_handle = vk::raii::SwapchainKHR { *device, sw.swapchain };
    m_extent = sw.extent;
    m_presentMode = static_cast<vk::PresentModeKHR>(sw.present_mode);
    m_surfaceFormat = { sw.image_format, sw.color_space };

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
        m_images.emplace_back(*m_deviceContext, image,
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

std::pair<vk::Result, std::uint32_t> CSwapchain::AcquireImage(vk::Semaphore semaphore, vk::Fence fence) const {
    std::uint32_t imageIndex;
    vk::Result result = static_cast<vk::Result>(m_handle.getDispatcher()->vkAcquireNextImageKHR(
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

    const vk::raii::Queue& queue = *m_deviceContext->Device().PresentQueue();
    vk::Result result = static_cast<vk::Result>(queue.getDispatcher()->vkQueuePresentKHR(
        static_cast<VkQueue>(*queue),
        reinterpret_cast<VkPresentInfoKHR const*>(&presentInfo)
    ));

    return result;
}
}
