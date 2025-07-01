#include "swapchain.hpp"

#include "logging.hpp"
#include "render_context.hpp"

namespace {
vk::SurfaceFormatKHR ChooseSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats) {
    constexpr vk::SurfaceFormatKHR preferredSurfaceFormat { vk::Format::eR8G8B8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear };

    const auto formatIt = std::ranges::find(availableFormats, preferredSurfaceFormat);
    if (formatIt == availableFormats.end()) {
        Log::Warning(
            "Preferred surface format ({}, {}) is not available. Choosing ({}, {})",
            vk::to_string(preferredSurfaceFormat.format),
            vk::to_string(preferredSurfaceFormat.colorSpace),
            vk::to_string(availableFormats[0].format),
            vk::to_string(availableFormats[0].colorSpace)
        );

        return availableFormats[0];
    }

    return *formatIt;
}
}

namespace Vulkan {
CSwapchain::CSwapchain(
    const CRenderContext* context,
    const vk::SurfaceKHR& surface,
    const std::uint32_t imageCount,
    const vk::PresentModeKHR presentMode
) : m_context(context) {
    CreateSwapchain(surface, imageCount, presentMode);
    CreateImages();
}

void CSwapchain::CreateSwapchain(
    const vk::SurfaceKHR& surface,
    const std::uint32_t imageCount,
    const vk::PresentModeKHR presentMode,
    const vk::SwapchainKHR& oldSwapchain
) {
    const CDevice* device = m_context->GetDevice();
    const vk::Device deviceHandle = device->GetHandle();
    const vk::PhysicalDevice physicalDevice = m_context->GetPhysicalDevice()->GetHandle();

    //====================
    vk::SwapchainCreateInfoKHR createInfo;
    createInfo.pNext = nullptr;
    createInfo.surface = m_info.m_associatedSurface = surface;

    //====================
    const vk::SurfaceCapabilitiesKHR surfaceCapabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface);
    createInfo.minImageCount = m_info.m_imageCount = std::clamp(
        imageCount,
        surfaceCapabilities.minImageCount,
        surfaceCapabilities.maxImageCount ? surfaceCapabilities.maxImageCount : std::numeric_limits<std::uint32_t>::max()
    );

    //====================
    m_info.m_surfaceFormat = ChooseSurfaceFormat(physicalDevice.getSurfaceFormatsKHR(surface));
    createInfo.imageFormat = m_info.m_surfaceFormat.format;
    createInfo.imageColorSpace = m_info.m_surfaceFormat.colorSpace;

    m_info.m_extent = ChooseSurfaceExtent(surfaceCapabilities);
    createInfo.imageExtent = m_info.m_extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;

    //====================
    const std::array queueFamilyIndices {
        device->GetGraphicsQueue().m_familyIndex,
        device->GetPresentQueue().m_familyIndex
    };

    if (device->GetGraphicsQueue().m_familyIndex != device->GetPresentQueue().m_familyIndex) {
        createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
        createInfo.queueFamilyIndexCount = static_cast<std::uint32_t>(queueFamilyIndices.size());
        createInfo.pQueueFamilyIndices = queueFamilyIndices.data();
    } else {
        createInfo.imageSharingMode = vk::SharingMode::eExclusive;
    }

    //====================
    createInfo.preTransform = surfaceCapabilities.currentTransform;
    createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    createInfo.presentMode = m_info.m_presentMode = presentMode; // VSync
    createInfo.clipped = vk::True;
    createInfo.oldSwapchain = oldSwapchain;

    //====================
    m_handle = deviceHandle.createSwapchainKHR(createInfo);
}

void CSwapchain::CreateImages() {
    const vk::Device deviceHandle = m_context->GetDevice()->GetHandle();

    m_images = deviceHandle.getSwapchainImagesKHR(m_handle);
    m_info.m_imageCount = static_cast<std::uint32_t>(m_images.size());
    m_imageViews.reserve(m_info.m_imageCount);

    for (const auto& image : m_images) {
        vk::ImageViewCreateInfo imageViewInfo {};
        imageViewInfo.image = image;
        imageViewInfo.viewType = vk::ImageViewType::e2D;
        imageViewInfo.format = m_info.m_surfaceFormat.format;
        imageViewInfo.components.r = vk::ComponentSwizzle::eIdentity;
        imageViewInfo.components.g = vk::ComponentSwizzle::eIdentity;
        imageViewInfo.components.b = vk::ComponentSwizzle::eIdentity;
        imageViewInfo.components.a = vk::ComponentSwizzle::eIdentity;
        imageViewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        imageViewInfo.subresourceRange.baseMipLevel = 0;
        imageViewInfo.subresourceRange.levelCount = 1;
        imageViewInfo.subresourceRange.baseArrayLayer = 0;
        imageViewInfo.subresourceRange.layerCount = 1;

        m_imageViews.emplace_back(deviceHandle.createImageView(imageViewInfo));
    }
}

void CSwapchain::DestroyImages() {
    const auto deviceHandle = m_context->GetDevice()->GetHandle();
    for (const auto& imageView : m_imageViews) {
        deviceHandle.destroyImageView(imageView);
    }
    m_imageViews.clear();
}

void CSwapchain::Recreate() {
    Recreate(m_info.m_associatedSurface, m_info.m_imageCount, m_info.m_presentMode);
}

void CSwapchain::Recreate(const vk::SurfaceKHR& surface, const std::uint32_t imageCount, const vk::PresentModeKHR presentMode) {
    const vk::Device deviceHandle = m_context->GetDevice()->GetHandle();

    deviceHandle.waitIdle();    // TODO: Wait for fence, not idle

    const vk::SwapchainKHR oldSwapchain = m_handle;
    CreateSwapchain(surface, imageCount, presentMode, oldSwapchain);
    deviceHandle.destroySwapchainKHR(oldSwapchain);

    DestroyImages();
    CreateImages();
}

vk::Extent2D CSwapchain::ChooseSurfaceExtent(const vk::SurfaceCapabilitiesKHR& capabilities) const {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    }

    int width, height;
    m_context->GetWindow()->GetDrawableSize(&width, &height);

    vk::Extent2D actualExtent = {
        static_cast<std::uint32_t>(width),
        static_cast<std::uint32_t>(height)
    };

    actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    return actualExtent;
}

CSwapchain::~CSwapchain() {
    if (!m_handle) {
        return;
    }

    const vk::Device device = m_context->GetDevice()->GetHandle();

    for (const auto& imageView : m_imageViews) {
        device.destroyImageView(imageView);
    }

    device.destroySwapchainKHR(m_handle);
}
}
