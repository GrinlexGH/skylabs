#include <VkBootstrap.h>

#include "skylabs/engine/logging.hpp"
#include "skylabs/engine/render/vulkan/swapchain.hpp"

namespace {
vk::SurfaceCapabilitiesKHR SurfaceCapabilities(const vk::raii::PhysicalDevice& physicalDevice,
                                               const vk::raii::SurfaceKHR& surface) {
    const vk::PhysicalDeviceSurfaceInfo2KHR surfaceInfo { *surface };
    return physicalDevice.getSurfaceCapabilities2KHR(surfaceInfo).surfaceCapabilities;
}

vk::Extent2D SurfaceExtent(const vk::SurfaceCapabilitiesKHR& caps, const sk::IWindow* window) {
    vk::Extent2D surfaceExtent = caps.currentExtent;
    if (surfaceExtent.width == std::numeric_limits<std::uint32_t>::max()) {
        const auto [width, height] = window->DrawableSize();
        surfaceExtent = vk::Extent2D { width, height };
    }

    assert(surfaceExtent.height != 0 && surfaceExtent.width != 0);

    return surfaceExtent;
}
}

namespace sk::render::vulkan {
struct Swapchain::InitData {
    const vk::raii::PhysicalDevice* physicalDevice;
    const Device* device;
    const Queues* queues;
    const IWindow* window;
    const vk::raii::SurfaceKHR* surface;
    vk::raii::SwapchainKHR swapchain;
    vk::SurfaceFormatKHR surfaceFormat;
    vk::SurfaceTransformFlagsKHR surfaceTransform;
    vk::Extent2D extent;
    vk::PresentModeKHR presentMode;
    std::vector<Attachment> images;
    std::vector<vk::raii::Semaphore> renderFinishedSemaphores;
};

Swapchain::Swapchain(InitData&& initData)
    : m_physicalDevice(initData.physicalDevice),
      m_device(initData.device),
      m_queues(initData.queues),
      m_window(initData.window),
      m_surface(initData.surface),
      m_handle(std::move(initData.swapchain)),
      m_surfaceFormat(initData.surfaceFormat),
      m_surfaceTransform(initData.surfaceTransform),
      m_extent(initData.extent),
      m_presentMode(initData.presentMode),
      m_images(std::move(initData.images)),
      m_renderFinishedSemaphores(std::move(initData.renderFinishedSemaphores)) { }

Swapchain::InitData Swapchain::Initialize(const vk::raii::PhysicalDevice& physicalDevice,
                                          const Device& device, const Queues& queues,
                                          const IWindow* window, const vk::raii::SurfaceKHR& surface,
                                          const std::uint32_t imageCount, vk::PresentModeKHR presentMode,
                                          VkSwapchainKHR oldHandle) {
    const vk::SurfaceCapabilitiesKHR caps = SurfaceCapabilities(physicalDevice, surface);
    vk::Extent2D extent = SurfaceExtent(caps, window);

    vkb::SwapchainBuilder swapchainBuilder { *physicalDevice, **device, *surface,
                                             queues.graphics.FamilyIndex(),
                                             queues.present.FamilyIndex() };

    auto swapchainResult =
        swapchainBuilder.set_old_swapchain(oldHandle)
            .set_desired_format(
                vk::SurfaceFormatKHR { vk::Format::eR8G8B8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear })
            .add_fallback_format(
                vk::SurfaceFormatKHR { vk::Format::eB8G8R8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear })
            .set_desired_present_mode(static_cast<VkPresentModeKHR>(presentMode))
            .add_fallback_present_mode(static_cast<VkPresentModeKHR>(vk::PresentModeKHR::eMailbox))
            .add_fallback_present_mode(static_cast<VkPresentModeKHR>(vk::PresentModeKHR::eFifo))
            .use_default_image_usage_flags()
            .set_desired_extent(extent.width, extent.height)
            .set_desired_min_image_count(imageCount)
            .set_pre_transform_flags(static_cast<VkSurfaceTransformFlagBitsKHR>(caps.currentTransform))
            .build();

    if (!swapchainResult) {
        const std::string message =
            fmt::format("Failed to create vulkan device ({}): {}", swapchainResult.error().message(),
                        fmt::join(swapchainResult.detailed_failure_reasons(), "; "));
        throw std::runtime_error { message };
    }

    vkb::Swapchain vkbSwapchain = swapchainResult.value();

    vk::raii::SwapchainKHR swapchain { *device, vkbSwapchain.swapchain };
    vk::SurfaceFormatKHR surfaceFormat { vk::Format { vkbSwapchain.image_format },
                                         vk::ColorSpaceKHR { vkbSwapchain.color_space } };
    vk::SurfaceTransformFlagsKHR surfaceTransform = caps.currentTransform;
    extent = vkbSwapchain.extent;
    presentMode = vk::PresentModeKHR { vkbSwapchain.present_mode };

    const std::vector<vk::Image> rawImages = swapchain.getImages();
    std::vector<Attachment> images;
    images.reserve(rawImages.size());
    std::vector<vk::raii::Semaphore> renderFinishedSemaphores;
    renderFinishedSemaphores.reserve(rawImages.size());

    for (auto& image : rawImages) {
        images.emplace_back(*device, image, vk::Extent3D { extent, 1 }, surfaceFormat.format, 1, 1,
                            vk::SampleCountFlagBits::e1, vk::ImageViewType::e2D);
        renderFinishedSemaphores.emplace_back(*device, vk::SemaphoreCreateInfo { });
    }

    return InitData { .physicalDevice = &physicalDevice,
                      .device = &device,
                      .queues = &queues,
                      .window = window,
                      .surface = &surface,
                      .swapchain = std::move(swapchain),
                      .surfaceFormat = surfaceFormat,
                      .surfaceTransform = surfaceTransform,
                      .extent = extent,
                      .presentMode = presentMode,
                      .images = std::move(images),
                      .renderFinishedSemaphores = std::move(renderFinishedSemaphores) };
}

Swapchain::Swapchain(const vk::raii::PhysicalDevice& physicalDevice, const Device& device,
                     const Queues& queues, const IWindow* window, const vk::raii::SurfaceKHR& surface,
                     const std::uint32_t imageCount, const vk::PresentModeKHR presentMode,
                     const VkSwapchainKHR oldHandle)
    : Swapchain(Initialize(physicalDevice, device, queues, window, surface, imageCount, presentMode,
                           oldHandle)) { }

Swapchain::Swapchain(const Swapchain& oldSwapchain, const SwapchainRecreateInfo& recreationInfo)
    : Swapchain(*oldSwapchain.m_physicalDevice, *oldSwapchain.m_device, *oldSwapchain.m_queues,
                oldSwapchain.m_window, *oldSwapchain.m_surface,
                recreationInfo.imageCount.value_or(oldSwapchain.m_images.size()),
                recreationInfo.presentMode.value_or(oldSwapchain.PresentMode()),
                *oldSwapchain.m_handle) { }

void Swapchain::Clear() {
    m_handle.clear();
    m_images.clear();
    m_renderFinishedSemaphores.clear();
}

bool Swapchain::IsOutdated() const {
    // Is the check too crude?
    const vk::SurfaceCapabilitiesKHR caps = SurfaceCapabilities(*m_physicalDevice, *m_surface);
    return SurfaceExtent(caps, m_window) != m_extent || caps.currentTransform != m_surfaceTransform;
}
}
