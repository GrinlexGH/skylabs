#include "skylabs/engine/render/vulkan/renderer.hpp"
#include "skylabs/engine/logging.hpp"

namespace sk::render::vulkan {
Renderer::Renderer(const IWindow* const window, const IOSAdapter* const osAdapter,
                   const filesystem::Filesystem& filesystem) {
    m_context = Context { window, osAdapter };
    m_swapchain = Swapchain { m_context.GetDevice(), window, *m_context.GetSurface(),
                              kFramesInFlightCount, vk::PresentModeKHR::eMailbox };

    m_inFlightContext = InFlightContext { kFramesInFlightCount };
    m_firstUse = InFlight<bool> { m_inFlightContext, true };
    m_fence = InFlight<vk::raii::Fence> { m_inFlightContext, *m_context.GetDevice(),
                                          vk::FenceCreateInfo { vk::FenceCreateFlagBits::eSignaled } };
    m_imageAvailableSemaphore =
        InFlight<vk::raii::Semaphore> { m_inFlightContext, *m_context.GetDevice(),
                                        vk::SemaphoreCreateInfo { } };

    const std::size_t imageCount = m_swapchain.Images().size();
    m_imageRenderFinishedSemaphores.reserve(imageCount);
    for (auto i = 0u; i < imageCount; ++i) {
        m_imageRenderFinishedSemaphores.emplace_back(*m_context.GetDevice(),
                                                     vk::SemaphoreCreateInfo { });
    }
}

Renderer::~Renderer() {
    try {
        m_context.GetDevice()->waitIdle();
    } catch (const vk::SystemError& e) {
        log::Error("Failed to destroy renderer: {}", e.what());
    }
}

void Renderer::BeginFrame() {
    std::ignore = m_context.GetDevice()->waitForFences({ m_fence.Get() }, vk::True,
                                                       std::numeric_limits<std::uint64_t>::max());

    // Acquire next image from the swapchain
    if (auto [acquireResult, imageIndex] = m_swapchain.AcquireImage(*m_imageAvailableSemaphore.Get());
        acquireResult != vk::Result::eSuccess) {
        log::Debug("Acquire result: {}", vk::to_string(acquireResult));

        // TODO: recursion?
        if (acquireResult == vk::Result::eErrorOutOfDateKHR) {
            RecreateSwapchain();
            std::tie(acquireResult, imageIndex) =
                m_swapchain.AcquireImage(*m_imageAvailableSemaphore.Get());
        }

#ifdef PLATFORM_WINDOWS
        if (acquireResult == vk::Result::eSuboptimalKHR) {
            RecreateSwapchain();
            // Suboptimal is a success result, so semaphore will be in use. We need to recreate it
            m_imageAvailableSemaphore.Get() =
                vk::raii::Semaphore { *m_context.GetDevice(), vk::SemaphoreCreateInfo { } };
            std::tie(acquireResult, imageIndex) =
                m_swapchain.AcquireImage(*m_imageAvailableSemaphore.Get());
        }
#endif

        if (acquireResult == vk::Result::eErrorSurfaceLostKHR) {
            return;
        }
    }

    // Reset fence after resizing to avoid deadlock on next invocation of Draw()
    m_context.GetDevice()->resetFences({ m_fence.Get() });
}

void Renderer::Draw(const glm::mat4 view, const float fov, float /*deltatime*/) { }

void Renderer::EndFrame() { m_inFlightContext.NextFrame(); }

void Renderer::OnPossibleSwapchainResize() {
    if (const auto [width, height] = m_context.Window()->DrawableSize();
        vk::Extent2D { width, height } != m_swapchain.Extent()) {
        RecreateSwapchain();
    }
}

void Renderer::RecreateSwapchain() {
    m_context.GetDevice()->waitIdle();
    m_swapchain.Recreate({ });
}
}
