#include <thread>

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

#include "skylabs/engine/logging.hpp"
#include "skylabs/engine/render/vulkan/descriptor_writer.hpp"
#include "skylabs/engine/render/vulkan/graphics_pipeline.hpp"
#include "skylabs/engine/render/vulkan/renderer.hpp"

namespace {
struct ViewProjection {
    glm::mat4 view { 1 };
    glm::mat4 projection { 1 };
};

glm::mat4 ReverseZPerspective(const unsigned int width, const unsigned int height, const float fov = 90,
                              const float nearZ = 0.01f) {
    glm::mat4 proj = glm::mat4(0.0f);
    const float g = 1.0f / std::tan(0.5f * glm::radians(fov));
    proj[0][0] = g / (static_cast<float>(width) / static_cast<float>(height));
    proj[1][1] = -g;
    proj[2][3] = -1.0f;
    proj[3][2] = nearZ;

    return proj;
}
}

namespace sk::render::vulkan {
Renderer::Renderer(const IWindow* const window, const IOSAdapter* const osAdapter,
                   const filesystem::Filesystem& filesystem) {
    // General context
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

    // Pipeline creation
    m_pipelineLayoutCache = PipelineLayoutCache { *m_context.GetDevice() };
    m_descriptorLayoutCache = DescriptorLayoutCache { *m_context.GetDevice() };
    m_descriptorAllocator = DescriptorAllocator { *m_context.GetDevice() };

    // Base pipeline
    m_viewProjection =
        InFlight<Buffer> { m_inFlightContext, *m_context.GetAllocator(), sizeof(ViewProjection),
                           vk::BufferUsageFlagBits::eUniformBuffer, MemoryLocation::eHostVisible };

    const vk::raii::DescriptorSetLayout& descriptorSetLayout = m_descriptorLayoutCache.GetLayout({
        { 0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex },
    });

    m_descriptorSet =
        InFlight { m_inFlightContext, m_descriptorAllocator.Allocate(std::vector(
                                          m_inFlightContext.FrameCount(), *descriptorSetLayout)) };

    DescriptorWriter descriptorWriter { *m_context.GetDevice() };
    for (const auto i : utils::Range(m_inFlightContext.FrameCount())) {
        descriptorWriter.Clear();
        descriptorWriter
            .WriteBuffer(0, *m_viewProjection[i], m_viewProjection[i].Size(), 0,
                         vk::DescriptorType::eUniformBuffer)
            .UpdateSet(*m_descriptorSet[i]);
    }

    const Shader vert(*m_context.GetDevice(), vk::ShaderStageFlagBits::eVertex,
                      filesystem.LoadAsVector32("res://shaders/triangle.vert.spv"));
    const Shader frag(*m_context.GetDevice(), vk::ShaderStageFlagBits::eFragment,
                      filesystem.LoadAsVector32("res://shaders/triangle.frag.spv"));

    std::array colorFormats { m_swapchain.SurfaceFormat().format };
    m_pipeline = GraphicsPipeline {
        *m_context.GetDevice(),
        GraphicsPipelineCreateInfo {
            .layout = m_pipelineLayoutCache.GetLayout(
                { .descriptorSetLayouts = { descriptorSetLayout }, .pushConstants = { } }),
            .shaders = { &vert, &frag },
            .vertexBindings = { },
            .renderingInfo = { { }, colorFormats } }
    };
}

Renderer::~Renderer() {
    try {
        m_context.GetDevice()->waitIdle();
    } catch (const vk::SystemError& e) {
        log::Error("Failed to destroy renderer: {}", e.what());
    }
}

void Renderer::BeginFrame() { }

void Renderer::Draw(const glm::mat4 view, const float fov, float /*deltatime*/) {
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
            std::tie(acquireResult, m_currentImageIndex) =
                m_swapchain.AcquireImage(*m_imageAvailableSemaphore.Get());
            if (acquireResult != vk::Result::eSuccess && acquireResult != vk::Result::eSuboptimalKHR) {
                return;
            }
        }

        if (acquireResult == vk::Result::eErrorSurfaceLostKHR) {
            return;
        }
    }

    // Reset fence after resizing to avoid deadlock on next invocation of Draw()
    m_context.GetDevice()->resetFences({ m_fence.Get() });

    // Update model and view
    auto [width, height] = m_swapchain.Extent();

    // Rotate render if we need
    const vk::SurfaceTransformFlagBitsKHR surfaceTransform = m_swapchain.SurfaceTransform();
    glm::mat4 rotation { 1 };

    if (surfaceTransform == vk::SurfaceTransformFlagBitsKHR::eRotate90) {
        rotation = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0, 0, 1));
    } else if (surfaceTransform == vk::SurfaceTransformFlagBitsKHR::eRotate270) {
        rotation = glm::rotate(glm::mat4(1.0f), glm::radians(270.0f), glm::vec3(0, 0, 1));
    } else if (surfaceTransform == vk::SurfaceTransformFlagBitsKHR::eRotate180) {
        rotation = glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0, 0, 1));
    }

    const ViewProjection viewProjection {
        .view = view,
        .projection = rotation * ReverseZPerspective(width, height, fov),
    };

    std::memcpy(m_viewProjection.Get().Data(), &viewProjection, sizeof(viewProjection));

    // Render frame
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
    attachInfo.clearValue.color = std::array { 0.1f, 0.0f, 0.0f, 1.0f };

    vk::RenderingInfo renderInfo { };
    renderInfo.renderArea = vk::Rect2D { { 0, 0 }, swapchainImage.Extent2D() };
    renderInfo.layerCount = 1;
    renderInfo.colorAttachmentCount = 1;
    renderInfo.pColorAttachments = &attachInfo;

    cmd->beginRendering(renderInfo);

    cmd->bindPipeline(vk::PipelineBindPoint::eGraphics, *m_pipeline);
    cmd->setViewport(0, { { 0.0f, 0.0f, static_cast<float>(swapchainImage.Extent().width),
                            static_cast<float>(swapchainImage.Extent().height), 0.0f, 1.0f } });
    cmd->setScissor(0, { { { 0, 0 }, swapchainImage.Extent2D() } });
    cmd->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_pipeline.Layout(), 0,
                            *m_descriptorSet.Get(), { });
    cmd->draw(3, 1, 0, 0);

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

        if (presentResult == vk::Result::eErrorOutOfDateKHR ||
            presentResult == vk::Result::eSuboptimalKHR) {
            OnPossibleSwapchainResize();
        }
    }

    m_inFlightContext.NextFrame();
}

void Renderer::EndFrame() { }

void Renderer::OnPossibleSwapchainResize() {
    if (m_swapchain.SurfaceExtent() != m_swapchain.Extent()) {
        m_context.GetDevice()->waitIdle();
        RecreateSwapchain();
    }
}

void Renderer::OnDeviceReset() {
    m_context.GetDevice()->waitIdle();
    m_swapchain.Clear();
    m_context.RecreateSurface();
    RecreateSwapchain();
}

void Renderer::RecreateSwapchain() { m_swapchain = Swapchain { m_swapchain, { } }; }
}
