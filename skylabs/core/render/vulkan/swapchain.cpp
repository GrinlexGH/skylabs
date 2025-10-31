#include <skylabs/core/render/vulkan/swapchain.hpp>

#include <skylabs/public/logging.hpp>

namespace {
auto ChooseSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats) -> vk::SurfaceFormatKHR {
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
    const CContext& context,
    const vk::SurfaceKHR& surface,
    const std::uint32_t imageCount,
    const vk::PresentModeKHR presentMode
) : m_context(&context) {
    Recreate(surface, imageCount, presentMode);
}

auto CSwapchain::CreateSwapchain(
    const vk::SurfaceKHR& surface,
    const std::uint32_t imageCount,
    vk::PresentModeKHR presentMode,
    const vk::raii::SwapchainKHR& oldSwapchain
) -> void {
    const CDevice& device = m_context->GetDevice();
    const vk::raii::PhysicalDevice physicalDevice = m_context->GetPhysicalDevice()->GetHandle();

    //====================
    vk::SwapchainCreateInfoKHR createInfo;
    createInfo.pNext = nullptr;
    createInfo.surface = m_associatedSurface = surface;

    //====================
    const vk::SurfaceCapabilitiesKHR surfaceCapabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface);
    createInfo.minImageCount = std::clamp(
        imageCount,
        surfaceCapabilities.minImageCount,
        surfaceCapabilities.maxImageCount ? surfaceCapabilities.maxImageCount : std::numeric_limits<std::uint32_t>::max()
    );

    //====================
    m_surfaceFormat = ChooseSurfaceFormat(physicalDevice.getSurfaceFormatsKHR(surface));
    createInfo.imageFormat = m_surfaceFormat.format;
    createInfo.imageColorSpace = m_surfaceFormat.colorSpace;

    m_extent = ChooseSurfaceExtent(surfaceCapabilities);
    createInfo.imageExtent = m_extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;

    //====================
    const std::array queueFamilyIndices {
        device.GetGraphicsQueue().m_familyIndex,
        device.GetPresentQueue().m_familyIndex
    };

    if (device.GetGraphicsQueue().m_familyIndex != device.GetPresentQueue().m_familyIndex) {
        createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
        createInfo.queueFamilyIndexCount = static_cast<uint32_t>(queueFamilyIndices.size());
        createInfo.pQueueFamilyIndices = queueFamilyIndices.data();
    } else {
        createInfo.imageSharingMode = vk::SharingMode::eExclusive;
    }

    //====================
    createInfo.preTransform = surfaceCapabilities.currentTransform;
    createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;

    //====================
    if (const std::vector<vk::PresentModeKHR> presentModes = physicalDevice.getSurfacePresentModesKHR(surface);
        !std::ranges::contains(presentModes, presentMode)
    ) {
        Log::Warning(
            "Requested present mode ({}) is not available. Choosing ({})",
            vk::to_string(presentMode),
            vk::to_string(presentModes[0])
        );
        presentMode = presentModes[0];
    }

    createInfo.presentMode = m_presentMode = presentMode; // VSync

    //====================
    createInfo.clipped = vk::True;
    createInfo.oldSwapchain = oldSwapchain;

    //====================
    m_handle = (*device).createSwapchainKHR(createInfo);
}

auto CSwapchain::CreateImages() -> void {
    m_images = m_handle.getImages();
    m_imageViews.reserve(m_images.size());

    for (const auto& image : m_images) {
        vk::ImageViewCreateInfo imageViewInfo {};
        imageViewInfo.image = image;
        imageViewInfo.viewType = vk::ImageViewType::e2D;
        imageViewInfo.format = m_surfaceFormat.format;
        imageViewInfo.components.r = vk::ComponentSwizzle::eIdentity;
        imageViewInfo.components.g = vk::ComponentSwizzle::eIdentity;
        imageViewInfo.components.b = vk::ComponentSwizzle::eIdentity;
        imageViewInfo.components.a = vk::ComponentSwizzle::eIdentity;
        imageViewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        imageViewInfo.subresourceRange.baseMipLevel = 0;
        imageViewInfo.subresourceRange.levelCount = 1;
        imageViewInfo.subresourceRange.baseArrayLayer = 0;
        imageViewInfo.subresourceRange.layerCount = 1;

        m_imageViews.emplace_back((*m_context->GetDevice()).createImageView(imageViewInfo));
    }

    assert(m_images.size() == m_imageViews.size());
}

auto CSwapchain::DestroyImages() -> void {
    m_imageViews.clear();
}

auto CSwapchain::Recreate() -> void {
    Recreate(m_associatedSurface, GetImageCount(), m_presentMode);
}

auto CSwapchain::Recreate(
    const vk::SurfaceKHR& surface,
    const std::uint32_t imageCount,
    const vk::PresentModeKHR presentMode
) -> void {
    (*m_context->GetDevice()).waitIdle(); // TODO: Wait for fence, not idle

    CreateSwapchain(surface, imageCount, presentMode, vk::raii::SwapchainKHR { std::move(m_handle) });

    DestroyImages();
    CreateImages();
}

auto CSwapchain::ChooseSurfaceExtent(const vk::SurfaceCapabilitiesKHR& capabilities) const -> vk::Extent2D {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    }

    int width, height;
    m_context->GetWindow()->GetDrawableSize(width, height);

    vk::Extent2D actualExtent = {
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height)
    };

    actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    return actualExtent;
}
}
