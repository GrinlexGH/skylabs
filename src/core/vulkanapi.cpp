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

#ifdef NDEBUG
bool enableValidationLayers = false;
#else
bool enableValidationLayers = true;
#endif

const std::vector<const char*> g_validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> g_deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

//============
// CVulkanRenderer
CVulkanRenderer::~CVulkanRenderer() {
    Destroy();
}

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#endif

void CVulkanRenderer::Init(IWindow* window) {
    using namespace vulkan_initializer;

    if (!window->GetHandle()) {
        throw std::runtime_error("Cant initialize vulkan: window is nullptr!\n");
    }

    m_instance = CreateInstance(m_debugMessenger);
    m_surface = CreateSurface(m_instance, window);

    m_physicalDevice = PickPhysicalDevice(m_instance, m_surface);
    CQueueFamilyIndices queueIndices = FindQueueFamilies(m_physicalDevice, m_surface);

    m_device = CreateLogicalDevice(m_physicalDevice, queueIndices);
    m_graphicsQueue = m_device.getQueue(queueIndices.m_graphicsFamily.value(), 0);
    m_presentQueue = m_device.getQueue(queueIndices.m_presentFamily.value(), 0);

    CSwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(m_physicalDevice, m_surface);
    vk::SurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.m_formats);
    vk::PresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.m_presentModes);
    m_swapChainExtent = ChooseSwapExtent(window, swapChainSupport.m_capabilities);

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

    m_commandPool = CreateCommandPool(m_device, queueIndices);
    m_commandBuffer = CreateCommandBuffer(m_device, m_commandPool);

    m_imageAvailableSemaphore = m_device.createSemaphore(vk::SemaphoreCreateInfo {});
    m_renderFinishedSemaphore = m_device.createSemaphore(vk::SemaphoreCreateInfo {});
    m_inFlightFence = m_device.createFence(vk::FenceCreateInfo { vk::FenceCreateFlagBits::eSignaled });

    m_initialized = true;
}

void CVulkanRenderer::RecordCommandBuffer(
    vk::CommandBuffer commandBuffer,
    std::vector<vk::Framebuffer> frameBuffers,
    uint32_t imageIndex,
    vk::RenderPass renderPass,
    vk::Extent2D extent,
    vk::Pipeline pipeline
) {
    vk::CommandBufferBeginInfo beginInfo {};
    beginInfo.pInheritanceInfo = nullptr;
    m_commandBuffer.begin(beginInfo);

    vk::RenderPassBeginInfo renderPassInfo {};
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = frameBuffers[imageIndex];
    renderPassInfo.renderArea.offset = vk::Offset2D { 0, 0 };
    renderPassInfo.renderArea.extent = extent;
    vk::ClearValue clearColor {};
    clearColor.color = std::array<float, 4>({ { 0.01f, 0.01f, 0.033f, 1.0f } });
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;
    commandBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);

    vk::Viewport viewport {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(extent.width);
    viewport.height = static_cast<float>(extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    commandBuffer.setViewport(0, viewport);

    vk::Rect2D scissor {};
    scissor.offset = vk::Offset2D { 0, 0 };
    scissor.extent = extent;
    commandBuffer.setScissor(0, scissor);

    commandBuffer.draw(3, 1, 0, 0);

    commandBuffer.endRenderPass();
    commandBuffer.end();
}

void CVulkanRenderer::Draw() {
    std::ignore = m_device.waitForFences(m_inFlightFence, vk::True, std::numeric_limits<unsigned int>::max());
    m_device.resetFences(m_inFlightFence);

    uint32_t imageIndex;
    std::ignore = m_device.acquireNextImageKHR(
        m_swapChain,
        std::numeric_limits<unsigned int>::max(),
        m_imageAvailableSemaphore,
        m_inFlightFence,
        &imageIndex
    );

    m_commandBuffer.reset();

    RecordCommandBuffer(m_commandBuffer, m_frameBuffers, 0, m_renderPass, m_swapChainExtent, m_pipeline);

    vk::SubmitInfo submitInfo {};

    vk::Semaphore waitSemaphores[] = { m_imageAvailableSemaphore };
    vk::PipelineStageFlags waitStages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_commandBuffer;
    vk::Semaphore signalSemaphores[] = { m_renderFinishedSemaphore };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;
    m_graphicsQueue.submit(submitInfo);

    vk::PresentInfoKHR presentInfo {};
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    vk::SwapchainKHR swapChains[] = { m_swapChain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr; // Optional

    std::ignore = m_presentQueue.presentKHR(presentInfo);
}

void CVulkanRenderer::WaitIdle() {
    m_device.waitIdle();
}

void CVulkanRenderer::Destroy() {
    if (!m_initialized)
        return;

    m_device.destroySemaphore(m_imageAvailableSemaphore);
    m_device.destroySemaphore(m_renderFinishedSemaphore);
    m_device.destroyFence(m_inFlightFence);

    m_device.destroyCommandPool(m_commandPool);

    for (auto framebuffer : m_frameBuffers) {
        m_device.destroyFramebuffer(framebuffer);
    }

    m_device.destroyPipeline(m_pipeline);
    m_device.destroyPipelineLayout(m_pipelineLayout);
    m_device.destroyRenderPass(m_renderPass);

    for (const auto& imageView : m_imageViews) {
        m_device.destroyImageView(imageView);
    }

    m_device.destroySwapchainKHR(m_swapChain);
    m_device.destroy();
    m_instance.destroySurfaceKHR(m_surface);
    if (enableValidationLayers) {
        m_instance.destroyDebugUtilsMessengerEXT(m_debugMessenger);
    }
    m_instance.destroy();
}
