#include "swapchain.hpp"

#include "console.hpp"
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
    const vk::PresentModeKHR& vSync,
    const vk::SwapchainKHR& oldSwaphchain
) : m_context(context)
{
    vk::SwapchainCreateInfoKHR createInfo;
    createInfo.pNext = nullptr;
    createInfo.surface = surface;

    //====================
    const vk::SurfaceCapabilitiesKHR surfaceCapabilities = m_context->PhysicalDevice()->GetHandle().getSurfaceCapabilitiesKHR(surface);
    createInfo.minImageCount = std::clamp(
        imageCount,
        surfaceCapabilities.minImageCount,
        surfaceCapabilities.maxImageCount ? surfaceCapabilities.maxImageCount : std::numeric_limits<std::uint32_t>::max()
    );

    //====================
    const vk::SurfaceFormatKHR chosenSurfaceFormat = ChooseSurfaceFormat(m_context->PhysicalDevice()->GetHandle().getSurfaceFormatsKHR(surface));
    createInfo.imageFormat = chosenSurfaceFormat.format;
    createInfo.imageColorSpace = chosenSurfaceFormat.colorSpace;
    createInfo.imageExtent = surfaceCapabilities.currentExtent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;

    //====================
    const std::uint32_t queueFamilyIndices[] = {
        m_context->Device()->GetGraphicsQueue().GetFamilyIndex(),
        m_context->Device()->GetPresentQueue().GetFamilyIndex()
    };

    if (m_context->Device()->GetGraphicsQueue().GetFamilyIndex() != m_context->Device()->GetPresentQueue().GetFamilyIndex()) {
        createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = vk::SharingMode::eExclusive;
    }

    //====================
    createInfo.preTransform = surfaceCapabilities.currentTransform;
    createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    createInfo.presentMode = vSync; // Vsync
    createInfo.clipped = vk::True;
    createInfo.oldSwapchain = oldSwaphchain;

    m_handle = m_context->Device()->GetHandle().createSwapchainKHR(createInfo);
}

CSwapchain::~CSwapchain() {
    if (!m_handle) {
        return;
    }

    m_context->Device()->GetHandle().destroySwapchainKHR(m_handle);
}
}
