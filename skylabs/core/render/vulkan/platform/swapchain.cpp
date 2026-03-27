#include <skylabs/core/render/vulkan/platform/swapchain.hpp>

#include <skylabs/public/logging.hpp>

namespace {
vk::SurfaceFormatKHR ChooseSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats) {
    constexpr vk::SurfaceFormatKHR preferredSurfaceFormat { vk::Format::eR8G8B8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear };

    const auto formatIt = std::ranges::find(availableFormats, preferredSurfaceFormat);
    if (formatIt == availableFormats.end()) {
        Log::Warning(
            "Preferred surface format ({}, {}) is not available. Choosing ({}, {})",
            vk::to_string(preferredSurfaceFormat.format),
            vk::to_string(preferredSurfaceFormat.colorSpace),
            vk::to_string(availableFormats.begin()->format),
            vk::to_string(availableFormats.begin()->colorSpace)
        );

        return *availableFormats.begin();
    }

    return *formatIt;
}
}

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
    const CPhysicalDevice& physicalDevice = m_context->PhysicalDevice();

    assert(device.IsExtensionEnabled(vk::KHRSwapchainExtensionName));

    vk::SwapchainCreateInfoKHR createInfo;
    createInfo.surface = surface;

    //====================
    const vk::SurfaceCapabilitiesKHR surfaceCapabilities = physicalDevice->getSurfaceCapabilitiesKHR(surface);
    createInfo.minImageCount = ChooseImageCount(surfaceCapabilities, imageCount);

    m_surfaceFormat = ChooseSurfaceFormat(physicalDevice->getSurfaceFormatsKHR(surface));
    createInfo.imageFormat = m_surfaceFormat.format;
    createInfo.imageColorSpace = m_surfaceFormat.colorSpace;

    createInfo.imageExtent = m_extent = ChooseSurfaceExtent(surfaceCapabilities);
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;

    //====================
    const std::array queueFamilyIndices {
        device.GraphicsQueue().FamilyIndex(),
        device.PresentQueue().FamilyIndex()
    };

    if (device.GraphicsQueue().FamilyIndex() != device.PresentQueue().FamilyIndex()) {
        createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
        createInfo.setQueueFamilyIndices({ queueFamilyIndices });
    } else {
        createInfo.imageSharingMode = vk::SharingMode::eExclusive;
    }

    //====================
    createInfo.preTransform = surfaceCapabilities.currentTransform;
    createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    createInfo.presentMode = m_presentMode = ChoosePresentMode(presentMode);
    createInfo.clipped = vk::True;
    createInfo.oldSwapchain = oldSwapchain;
    createInfo.pNext = nullptr;

    m_handle = vk::raii::SwapchainKHR { *device, createInfo };
    CreateImages();
}

void CSwapchain::CreateImages() {
    m_images.clear();
    for (auto& image : m_handle.getImages()) {
        m_images.emplace_back(*m_context, image, vk::Extent3D { m_extent, 1 }, m_surfaceFormat.format, 1, 1, vk::SampleCountFlagBits::e1);
    }
}

vk::Extent2D CSwapchain::ChooseSurfaceExtent(const vk::SurfaceCapabilitiesKHR& capabilities) const {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    }

    auto [width, height] = m_context->Window()->DrawableSize();

    vk::Extent2D actualExtent = { width, height };
    actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    return actualExtent;
}

std::uint32_t CSwapchain::ChooseImageCount(const vk::SurfaceCapabilitiesKHR& capabilities, std::uint32_t requestedCount) const {
    return std::clamp(
        requestedCount,
        capabilities.minImageCount,
        capabilities.maxImageCount ? capabilities.maxImageCount : std::numeric_limits<std::uint32_t>::max()
    );
}

vk::PresentModeKHR CSwapchain::ChoosePresentMode(const vk::PresentModeKHR requestedMode) const {
    const std::vector<vk::PresentModeKHR> availableModes =
        m_context->PhysicalDevice()->getSurfacePresentModesKHR(m_associatedSurface);

    if (std::ranges::contains(availableModes, requestedMode)) {
        return requestedMode;
    }

    if (std::ranges::contains(availableModes, vk::PresentModeKHR::eMailbox)) {
        Log::Warning("Requested mode not available. Falling back to Mailbox");
        return vk::PresentModeKHR::eMailbox;
    }

    if (std::ranges::contains(availableModes, vk::PresentModeKHR::eImmediate)) {
        Log::Warning("Requested mode not available. Falling back to Immediate");
        return vk::PresentModeKHR::eImmediate;
    }

    Log::Warning("Using standard FIFO (V-Sync)");
    return vk::PresentModeKHR::eFifo;
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
