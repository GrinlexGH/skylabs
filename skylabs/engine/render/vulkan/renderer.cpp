#include <thread>

#include "skylabs/engine/logging.hpp"
#include "skylabs/engine/render/vulkan/graphics_pipeline.hpp"
#include "skylabs/engine/render/vulkan/renderer.hpp"

namespace sk::render::vulkan {
Renderer::Renderer(const IWindow* const window, const IOSAdapter* const osAdapter,
                   const filesystem::Filesystem& /*filesystem*/) {
    m_context = Context { window, osAdapter };

    m_swapchain = Swapchain { m_context.GetDevice(), window, *m_context.GetSurface(),
                              kFramesInFlightCount, vk::PresentModeKHR::eMailbox };

    m_inFlightContext = InFlightContext { kFramesInFlightCount };

    m_commandBufferAllocator =
        CommandBufferAllocator { *m_context.GetDevice(),
                                 m_context.GetDevice().GraphicsQueue().FamilyIndex() };

    // Frame synchronization
    m_firstUse = InFlight<bool> { m_inFlightContext, true };
    m_fence = InFlight<vk::raii::Fence> { m_inFlightContext, *m_context.GetDevice(),
                                          vk::FenceCreateInfo { vk::FenceCreateFlagBits::eSignaled } };
    m_imageAvailableSemaphore =
        InFlight<vk::raii::Semaphore> { m_inFlightContext, *m_context.GetDevice(),
                                        vk::SemaphoreCreateInfo { } };

    m_commandBuffers = InFlight { m_inFlightContext,
                                  m_commandBufferAllocator.Allocate(vk::CommandBufferLevel::ePrimary,
                                                                    m_inFlightContext.FrameCount()) };
}

Renderer::~Renderer() {
    try {
        m_context.GetDevice()->waitIdle();
    } catch (const vk::SystemError& e) {
        log::Error("Failed to destroy renderer: {}", e.what());
    }
}

void Renderer::BeginFrame() { }

void Renderer::Draw(const glm::mat4 /*view*/, const float /*fov*/, float /*deltatime*/) {
    std::ignore = m_context.GetDevice()->waitForFences({ m_fence.Get() }, vk::True,
                                                       std::numeric_limits<std::uint64_t>::max());

    // Acquire next image from the swapchain
    vk::Result acquireResult;
    if (std::tie(acquireResult, m_currentImageIndex) =
            m_swapchain.AcquireImage(*m_imageAvailableSemaphore.Get());
        acquireResult != vk::Result::eSuccess) {
        log::Debug("Acquire result: {}", vk::to_string(acquireResult));

        if (acquireResult == vk::Result::eErrorOutOfDateKHR) {
            OnPossibleSwapchainResize();
        }

        if (acquireResult == vk::Result::eErrorSurfaceLostKHR) {
            return;
        }
    }

    // Reset fence after resizing to avoid deadlock on next invocation of Draw()
    m_context.GetDevice()->resetFences({ m_fence.Get() });

    const auto& cmd = m_commandBuffers[m_inFlightContext.InFlightIndex()];
    const auto& swapchainImage = m_swapchain.Images()[m_currentImageIndex];

    cmd->reset();
    cmd->begin({ });

    cmd.PipelineBarrier({
        ImageBarrier { .image = swapchainImage,
                       .range = swapchainImage.FullRange(),
                       .oldUsage = Usage::eNone,
                       .newUsage = Usage::eColorAttachment },
    });

    vk::RenderingAttachmentInfo attachInfo { };
    attachInfo.imageView = *swapchainImage.View();
    attachInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
    attachInfo.loadOp = vk::AttachmentLoadOp::eClear;
    attachInfo.storeOp = vk::AttachmentStoreOp::eStore;
    const float r = std::abs(
        std::sin(std::chrono::high_resolution_clock::now().time_since_epoch().count() * 0.000000001));
    attachInfo.clearValue.color = std::array { r, 0.1f, 0.12f, 1.0f };

    vk::RenderingInfo renderInfo { };
    renderInfo.renderArea = vk::Rect2D { { 0, 0 }, swapchainImage.Extent2D() };
    renderInfo.layerCount = 1;
    renderInfo.colorAttachmentCount = 1;
    renderInfo.pColorAttachments = &attachInfo;

    cmd->beginRendering(renderInfo);
    cmd->endRendering();

    cmd.PipelineBarrier({ ImageBarrier { .image = swapchainImage,
                                         .range = swapchainImage.FullRange(),
                                         .oldUsage = Usage::eColorAttachment,
                                         .newUsage = Usage::ePresent } });

    cmd->end();

    vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;

    vk::SubmitInfo finalSubmit { };
    finalSubmit.setWaitSemaphores({ *m_imageAvailableSemaphore.Get() });
    finalSubmit.setWaitDstStageMask({ waitStage });
    finalSubmit.setCommandBuffers({ **m_commandBuffers[m_inFlightContext.InFlightIndex()] });
    finalSubmit.setSignalSemaphores({ *m_swapchain.RenderFinishedSemaphores()[m_currentImageIndex] });
    m_context.GetDevice().GraphicsQueue()->submit(finalSubmit, m_fence.Get());

    const vk::Result presentResult = m_swapchain.PresentImage(
        m_currentImageIndex, { *m_swapchain.RenderFinishedSemaphores()[m_currentImageIndex] });
    if (presentResult != vk::Result::eSuccess) {
        log::Debug("Present result: {}", vk::to_string(presentResult));

        if (presentResult == vk::Result::eErrorOutOfDateKHR) {
            OnPossibleSwapchainResize();
        }

#ifdef PLATFORM_ANDROID
        if (presentResult == vk::Result::eSuboptimalKHR) {
            RecreateSwapchain();
        }
#endif
    }

    m_inFlightContext.NextFrame();
}

void Renderer::EndFrame() { }

void Renderer::OnPossibleSwapchainResize() { RecreateSwapchain(); }

void Renderer::OnDeviceReset() {
    m_context.GetDevice()->waitIdle();
    m_swapchain.Clear();
    m_context.RecreateSurface();
    RecreateSwapchain();
}

void Renderer::RecreateSwapchain() {
    if (const auto [width, height] = m_context.Window()->DrawableSize();
        vk::Extent2D { width, height } != m_swapchain.Extent() && width != 0 && height != 0) {
        m_context.GetDevice()->waitIdle();
        m_swapchain = Swapchain { m_swapchain, { } };
    }
}
}
