#include "swapchain.hpp"

#include "../renderer.hpp"
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
    const std::weak_ptr<CRenderContext>& context,
    const vk::SurfaceKHR& surface,
    const std::uint32_t imageCount,
    const vk::PresentModeKHR& vSync,
    const vk::SwapchainKHR& oldSwaphchain
) : m_context(context)
{
    const std::shared_ptr<CRenderContext> ctx = m_context.lock();
    if (!ctx) {
        throw CRendererInitError("Cannot create vulkan swapchain: context is expired!");
    }

    vk::SwapchainCreateInfoKHR createInfo;
    createInfo.pNext = nullptr;
    createInfo.surface = surface;

    //====================
    const vk::SurfaceCapabilitiesKHR surfaceCapabilities = ctx->PhysicalDevice()->GetHandle().getSurfaceCapabilitiesKHR(surface);
    createInfo.minImageCount = std::clamp(
        imageCount,
        surfaceCapabilities.minImageCount,
        surfaceCapabilities.maxImageCount ? surfaceCapabilities.maxImageCount : std::numeric_limits<std::uint32_t>::max()
    );

    //====================
    const vk::SurfaceFormatKHR chosenSurfaceFormat = ChooseSurfaceFormat(ctx->PhysicalDevice()->GetHandle().getSurfaceFormatsKHR(surface));
    createInfo.imageFormat = chosenSurfaceFormat.format;
    createInfo.imageColorSpace = chosenSurfaceFormat.colorSpace;
    createInfo.imageExtent = surfaceCapabilities.currentExtent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;

    //====================
    const std::uint32_t queueFamilyIndices[] = {
        ctx->Device()->GetGraphicsQueue().m_familyIndex,
        ctx->Device()->GetPresentQueue().m_familyIndex
    };

    if (ctx->Device()->GetGraphicsQueue().m_familyIndex != ctx->Device()->GetPresentQueue().m_familyIndex) {
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

    m_handle = ctx->Device()->GetHandle().createSwapchainKHR(createInfo);
}

CSwapchain::~CSwapchain() {
    if (!m_handle) {
        return;
    }

    if (const auto ctx = m_context.lock()) {
        ctx->Device()->GetHandle().destroySwapchainKHR(m_handle);
    } else {
        Log::Error("Couldn't properly destroy vulkan swapchain. Render context is expired!");
    }
}
}
