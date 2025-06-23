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
    const vk::PresentModeKHR presentMode,
    const vk::SwapchainKHR& oldSwaphchain
) : m_context(context)
{
    const CDevice* device = context->GetDevice();
    const vk::Device deviceHandle = device->GetHandle();
    const vk::PhysicalDevice physicalDevice = context->GetPhysicalDevice()->GetHandle();

    vk::SwapchainCreateInfoKHR createInfo;
    createInfo.pNext = nullptr;
    createInfo.surface = surface;

    //====================
    const vk::SurfaceCapabilitiesKHR surfaceCapabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface);
    createInfo.minImageCount = std::clamp(
        imageCount,
        surfaceCapabilities.minImageCount,
        surfaceCapabilities.maxImageCount ? surfaceCapabilities.maxImageCount : std::numeric_limits<std::uint32_t>::max()
    );

    //====================
    m_info.m_format = ChooseSurfaceFormat(physicalDevice.getSurfaceFormatsKHR(surface));
    createInfo.imageFormat = m_info.m_format.format;
    createInfo.imageColorSpace = m_info.m_format.colorSpace;

    m_info.m_extent = ChooseSurfaceExtent(surfaceCapabilities);
    createInfo.imageExtent = m_info.m_extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;

    //====================
    const std::uint32_t queueFamilyIndices[] {
        device->GetGraphicsQueue().m_familyIndex,
        device->GetPresentQueue().m_familyIndex
    };

    if (device->GetGraphicsQueue().m_familyIndex != device->GetPresentQueue().m_familyIndex) {
        createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = vk::SharingMode::eExclusive;
    }

    //====================
    createInfo.preTransform = surfaceCapabilities.currentTransform;
    createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    createInfo.presentMode = presentMode;   // VSync
    createInfo.clipped = vk::True;
    createInfo.oldSwapchain = oldSwaphchain;

    m_handle = deviceHandle.createSwapchainKHR(createInfo);

    m_images = deviceHandle.getSwapchainImagesKHR(m_handle);
    m_imageViews.reserve(m_images.size());

    for (const auto& image : m_images) {
        vk::ImageViewCreateInfo imageViewInfo {};
        imageViewInfo.image = image;
        imageViewInfo.viewType = vk::ImageViewType::e2D;
        imageViewInfo.format = m_info.m_format.format;
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
