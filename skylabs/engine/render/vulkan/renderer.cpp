#include <thread>

#include <glm/glm.hpp>
#include <glm/gtx/type_aligned.hpp>

#include "skylabs/engine/logging.hpp"
#include "skylabs/engine/render/vulkan/descriptor_writer.hpp"
#include "skylabs/engine/render/vulkan/graphics_pipeline.hpp"
#include "skylabs/engine/render/vulkan/renderer.hpp"

namespace {
struct ViewProjection {
    glm::aligned_mat4 view { 1 };
    glm::aligned_mat4 projection { 1 };
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

    // Base pipeline
    m_viewProjection =
        InFlight<Buffer> { m_inFlightContext, *m_context.GetAllocator(), sizeof(ViewProjection),
                           vk::BufferUsageFlagBits::eUniformBuffer, MemoryLocation::eHostVisible };

    const vk::raii::DescriptorSetLayout& descriptorSetLayout = m_descriptorLayoutCache.GetLayout({
        { 0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex },
    });

    // Схуяли я вообще должен заранее знать что и сколько я хочу выделить?!!
    vk::DescriptorPoolCreateInfo descriptorPoolCreateInfo;
    std::array descriptorPoolSizes { vk::DescriptorPoolSize { vk::DescriptorType::eUniformBuffer,
                                                              kFramesInFlightCount } };
    descriptorPoolCreateInfo.setPoolSizes(descriptorPoolSizes);
    descriptorPoolCreateInfo.setMaxSets(kFramesInFlightCount);
    m_descriptorPool = vk::raii::DescriptorPool { *m_context.GetDevice(), descriptorPoolCreateInfo };

    vk::DescriptorSetAllocateInfo descriptorSetAllocationInfo { };
    std::vector descriptorSetLayouts { m_inFlightContext.FrameCount(), *descriptorSetLayout };
    descriptorSetAllocationInfo.setDescriptorPool(m_descriptorPool);
    descriptorSetAllocationInfo.setSetLayouts(descriptorSetLayouts);
    std::vector<vk::DescriptorSet> descriptorSets =
        m_context.GetDevice()->allocateDescriptorSets(descriptorSetAllocationInfo) |
        std::views::transform([](vk::raii::DescriptorSet& set) { return set.release(); }) |
        std::ranges::to<std::vector<vk::DescriptorSet>>();
    m_descriptorSet = InFlight { m_inFlightContext, std::move(descriptorSets) };

    DescriptorWriter descriptorWriter { *m_context.GetDevice() };
    for (const auto i : utils::Range(m_inFlightContext.FrameCount())) {
        descriptorWriter.Clear();
        descriptorWriter
            .WriteBuffer(0, *m_viewProjection[i], m_viewProjection[i].Size(), 0,
                         vk::DescriptorType::eUniformBuffer)
            .UpdateSet(m_descriptorSet[i]);
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

void Renderer::BeginFrame() {
    if (m_frameActive || m_surfaceLost) return;

    try {
        // This part should handle suboptimal
        // May throw surface lost error
        if (m_swapchain.IsOutdated()) {
            m_context.GetDevice()->waitIdle();
            RecreateSwapchain();
        }

        std::ignore = m_context.GetDevice()->waitForFences({ m_fence.Get() }, vk::True,
                                                           std::numeric_limits<std::uint64_t>::max());

        // Acquire next image from the swapchain
        vk::Result result;
        std::tie(result, m_currentImageIndex) = m_swapchain->acquireNextImage(
            std::numeric_limits<std::uint64_t>::max(), *m_imageAvailableSemaphore.Get());

        if (result == vk::Result::eErrorOutOfDateKHR) {
            m_context.GetDevice()->waitIdle();
            RecreateSwapchain();

            std::tie(result, m_currentImageIndex) = m_swapchain->acquireNextImage(
                std::numeric_limits<std::uint64_t>::max(), *m_imageAvailableSemaphore.Get());

            // If we still can't restore the swapchain, we simply return
            if (result == vk::Result::eErrorOutOfDateKHR) {
                return;
            }
        }
    } catch (const vk::SurfaceLostKHRError&) {
        MarkSurfaceLost();
        return;
    }

    const auto& cmd = m_commandBuffers.Get();
    cmd->reset();
    cmd->begin({ vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
    cmd.PipelineBarrier({ ImageBarrier { .image = m_swapchain.Images()[m_currentImageIndex],
                                         .range = m_swapchain.Images()[m_currentImageIndex].FullRange(),
                                         .oldUsage = Usage::eSwapchainAcquire,
                                         .newUsage = Usage::eColorAttachment } });

    m_frameActive = true;
}

void Renderer::Draw(const glm::mat4 view, const float fov, float /*deltatime*/) {
    if (!m_frameActive) return;

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
    const auto& cmd = m_commandBuffers.Get();
    const auto& swapchainImage = m_swapchain.Images()[m_currentImageIndex];

    vk::RenderingAttachmentInfo attachInfo { };
    attachInfo.imageView = *swapchainImage.View();
    attachInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
    attachInfo.loadOp = vk::AttachmentLoadOp::eClear;
    attachInfo.storeOp = vk::AttachmentStoreOp::eStore;
    attachInfo.clearValue.color = std::array { 0.0f, 0.0f, 0.0f, 0.0f };

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
                            m_descriptorSet.Get(), { });
    cmd->draw(3, 1, 0, 0);

    cmd->endRendering();
}

void Renderer::EndFrame() {
    if (!m_frameActive) return;
    m_frameActive = false;

    const auto& cmd = m_commandBuffers.Get();
    cmd.PipelineBarrier({ ImageBarrier { .image = m_swapchain.Images()[m_currentImageIndex],
                                         .range = m_swapchain.Images()[m_currentImageIndex].FullRange(),
                                         .oldUsage = Usage::eColorAttachment,
                                         .newUsage = Usage::ePresent } });

    cmd->end();

    m_context.GetDevice()->resetFences({ m_fence.Get() });

    vk::SemaphoreSubmitInfo waitInfo { };
    waitInfo.setSemaphore(*m_imageAvailableSemaphore.Get());
    waitInfo.setStageMask(vk::PipelineStageFlagBits2::eColorAttachmentOutput);

    vk::SemaphoreSubmitInfo signalInfo { };
    signalInfo.setSemaphore(*m_swapchain.RenderFinishedSemaphores()[m_currentImageIndex]);
    signalInfo.setStageMask(vk::PipelineStageFlagBits2::eAllCommands);

    vk::CommandBufferSubmitInfo cmdInfo { };
    cmdInfo.setCommandBuffer(**m_commandBuffers.Get());

    vk::SubmitInfo2 finalSubmit { };
    finalSubmit.setWaitSemaphoreInfos(waitInfo);
    finalSubmit.setCommandBufferInfos(cmdInfo);
    finalSubmit.setSignalSemaphoreInfos(signalInfo);
    m_context.GetDevice().GraphicsQueue()->submit2(finalSubmit, m_fence.Get());

    m_inFlightContext.NextFrame();

    try {
        vk::PresentInfoKHR presentInfo { };
        presentInfo.setWaitSemaphores(*m_swapchain.RenderFinishedSemaphores()[m_currentImageIndex]);
        presentInfo.setSwapchains({ **m_swapchain });
        presentInfo.setImageIndices({ m_currentImageIndex });
        if (const vk::Result result = m_context.GetDevice().PresentQueue()->presentKHR(presentInfo);
            result == vk::Result::eErrorOutOfDateKHR) {
            m_context.GetDevice()->waitIdle();
            RecreateSwapchain();
        }
    } catch (const vk::SurfaceLostKHRError&) {
        MarkSurfaceLost();
    }
}

void Renderer::OnDeviceReset() {
    m_context.GetDevice()->waitIdle();
    m_swapchain.Clear();
    m_context.RecreateSurface();
    RecreateSwapchain();
    m_surfaceLost = false;
}

void Renderer::RecreateSwapchain() { m_swapchain = Swapchain { m_swapchain, { } }; }

void Renderer::MarkSurfaceLost() { m_surfaceLost = true; }
}
