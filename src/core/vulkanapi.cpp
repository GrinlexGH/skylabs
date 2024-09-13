#include "console.hpp"
#include "vulkanapi.hpp"
#include "SDL.hpp"
#include "SDL_Vulkan.hpp"
#include "unicode.hpp"
#include "resourceloader.hpp"
#include "vulkan_initializer.hpp"

#include <fstream>
#include <filesystem>
#include <set>

//============
// CVulkanRenderer
CVulkanRenderer::~CVulkanRenderer() {
    Destroy();
}

void CVulkanRenderer::Init(IWindow* window) {
    using namespace vk_initializer;

    if (!window->GetHandle()) {
        throw std::runtime_error("Cant initialize vulkan: window is nullptr!\n");
    }

    m_window = window;

    m_instance = CreateInstance(m_debugMessenger);
    m_surface = CreateSurface(m_instance, m_window);

    m_physicalDevice = PickPhysicalDevice(m_instance, m_surface);
    CQueueFamilyIndices queueIndices = FindQueueFamilies(m_physicalDevice, m_surface);

    m_device = CreateLogicalDevice(m_physicalDevice, queueIndices);
    m_graphicsQueue = m_device.getQueue(queueIndices.m_graphicsFamily.value(), 0);
    m_presentQueue = m_device.getQueue(queueIndices.m_presentFamily.value(), 0);

    CSwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(m_physicalDevice, m_surface);
    vk::SurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.m_formats);
    vk::PresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.m_presentModes);
    m_swapChainExtent = ChooseSwapExtent(m_window, swapChainSupport.m_capabilities);

    m_swapChain = CreateSwapChain(
        m_device, m_surface,
        queueIndices, swapChainSupport,
        surfaceFormat, presentMode, m_swapChainExtent
    );

    m_images = m_device.getSwapchainImagesKHR(m_swapChain);
    m_imageFormat = surfaceFormat.format;
    m_imageViews = CreateImageViews(m_device, m_images, m_imageFormat);

    m_renderPass = CreateRenderPass(m_device, m_imageFormat);
    m_pipelineLayout = CreatePipelineLayout(m_device);
    m_pipeline = CreatePipeline(m_device, m_pipelineLayout, m_renderPass);

    m_frameBuffers = CreateFramebuffers(m_device, m_imageViews, m_renderPass, m_swapChainExtent);

    m_commandPool = CreateCommandPool(m_device, queueIndices.m_graphicsFamily.value());
    m_commandBuffers = CreateCommandBuffers(m_device, m_commandPool);

    m_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    for (std::size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        m_imageAvailableSemaphores[i] = m_device.createSemaphore(vk::SemaphoreCreateInfo {});
        m_renderFinishedSemaphores[i] = m_device.createSemaphore(vk::SemaphoreCreateInfo {});
        m_inFlightFences[i] = m_device.createFence(vk::FenceCreateInfo { vk::FenceCreateFlagBits::eSignaled });
    }

    m_initialized = true;
}

void CVulkanRenderer::Draw() {
    std::ignore = m_device.waitForFences(m_inFlightFences[m_currentFrame], vk::True, std::numeric_limits<unsigned int>::max());

    uint32_t imageIndex;
    vk::Result res = m_device.acquireNextImageKHR(
        m_swapChain,
        std::numeric_limits<unsigned int>::max(),
        m_imageAvailableSemaphores[m_currentFrame],
        VK_NULL_HANDLE,
        &imageIndex
    );

    if (res == vk::Result::eErrorOutOfDateKHR) {
        RecreateSwapChain();
        return;
    }

    m_device.resetFences(m_inFlightFences[m_currentFrame]);

    m_commandBuffers[m_currentFrame].reset();

    vk::CommandBufferBeginInfo beginInfo {};
    beginInfo.pInheritanceInfo = nullptr;
    m_commandBuffers[m_currentFrame].begin(beginInfo);

    vk::RenderPassBeginInfo renderPassInfo {};
    renderPassInfo.renderPass = m_renderPass;
    renderPassInfo.framebuffer = m_frameBuffers[imageIndex];
    renderPassInfo.renderArea.offset = vk::Offset2D { 0, 0 };
    renderPassInfo.renderArea.extent = m_swapChainExtent;
    vk::ClearValue clearColor {};
    clearColor.color = std::array<float, 4>({ { 0.01f, 0.01f, 0.033f, 1.0f } });
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;
    m_commandBuffers[m_currentFrame].beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
    m_commandBuffers[m_currentFrame].bindPipeline(vk::PipelineBindPoint::eGraphics, m_pipeline);

    vk::Viewport viewport {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(m_swapChainExtent.width);
    viewport.height = static_cast<float>(m_swapChainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    m_commandBuffers[m_currentFrame].setViewport(0, viewport);

    vk::Rect2D scissor {};
    scissor.offset = vk::Offset2D { 0, 0 };
    scissor.extent = m_swapChainExtent;
    m_commandBuffers[m_currentFrame].setScissor(0, scissor);

    m_commandBuffers[m_currentFrame].draw(3, 1, 0, 0);

    m_commandBuffers[m_currentFrame].endRenderPass();
    m_commandBuffers[m_currentFrame].end();

    vk::SubmitInfo submitInfo {};

    vk::Semaphore waitSemaphores[] = { m_imageAvailableSemaphores[m_currentFrame] };
    vk::PipelineStageFlags waitStages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_commandBuffers[m_currentFrame];
    vk::Semaphore signalSemaphores[] = { m_renderFinishedSemaphores[m_currentFrame] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;
    m_graphicsQueue.submit(submitInfo, m_inFlightFences[m_currentFrame]);

    vk::PresentInfoKHR presentInfo {};
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    vk::SwapchainKHR swapChains[] = { m_swapChain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;

    res = m_presentQueue.presentKHR(&presentInfo);
    if (res == vk::Result::eErrorOutOfDateKHR || res == vk::Result::eSuboptimalKHR || m_frameBufferResized) {
        m_frameBufferResized = false;
        RecreateSwapChain();
    }
    m_currentFrame = (m_currentFrame + 1) % vk_initializer::MAX_FRAMES_IN_FLIGHT;
    m_device.waitIdle();
}

void CVulkanRenderer::RecreateSwapChain() {
    using namespace vk_initializer;

    vkDeviceWaitIdle(m_device);

    m_device.waitIdle();

    CleanupSwapChain();

    CQueueFamilyIndices queueIndices = FindQueueFamilies(m_physicalDevice, m_surface);
    CSwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(m_physicalDevice, m_surface);
    vk::SurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.m_formats);
    vk::PresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.m_presentModes);
    m_swapChainExtent = ChooseSwapExtent(m_window, swapChainSupport.m_capabilities);

    m_swapChain = CreateSwapChain(
        m_device, m_surface,
        queueIndices, swapChainSupport,
        surfaceFormat, presentMode, m_swapChainExtent
    );
    m_images = m_device.getSwapchainImagesKHR(m_swapChain);
    m_imageViews = CreateImageViews(m_device, m_images, m_imageFormat);
    m_frameBuffers = CreateFramebuffers(m_device, m_imageViews, m_renderPass, m_swapChainExtent);
}

void CVulkanRenderer::CleanupSwapChain() {
    for (size_t i = 0; i < m_frameBuffers.size(); ++i) {
        m_device.destroyFramebuffer(m_frameBuffers[i]);
    }

    for (size_t i = 0; i < m_imageViews.size(); ++i) {
        m_device.destroyImageView(m_imageViews[i]);
    }

    m_device.destroySwapchainKHR(m_swapChain);
}

void CVulkanRenderer::Destroy() {
    using namespace vk_initializer;
    if (!m_initialized) {
        return;
    }

    CleanupSwapChain();

    m_device.destroyPipeline(m_pipeline);
    m_device.destroyPipelineLayout(m_pipelineLayout);
    m_device.destroyRenderPass(m_renderPass);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        m_device.destroySemaphore(m_imageAvailableSemaphores[i]);
        m_device.destroySemaphore(m_renderFinishedSemaphores[i]);
        m_device.destroyFence(m_inFlightFences[i]);
    }

    m_device.destroyCommandPool(m_commandPool);

    m_device.destroy();
    m_instance.destroySurfaceKHR(m_surface);
    if (enableValidationLayers) {
        m_instance.destroyDebugUtilsMessengerEXT(m_debugMessenger);
    }
    m_instance.destroy();
}
