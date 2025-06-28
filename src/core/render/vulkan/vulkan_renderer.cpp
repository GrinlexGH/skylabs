#include "vulkan_renderer.hpp"

#include "logging.hpp"
#include "resource_system.hpp"

namespace {
std::vector<vk::Framebuffer> CreateFrameBuffers(
    vk::Device deviceHandle,
    const Vulkan::CSwapchain* swapchain,
    const vk::RenderPass& renderPass
) {
    const Vulkan::CSwapchain::CInfo& swapchainInfo = swapchain->GetInfo();

    std::vector<vk::Framebuffer> out;
    out.reserve(swapchainInfo.m_imageCount);
    for (std::size_t i = 0; i < swapchainInfo.m_imageCount; i++) {
        vk::FramebufferCreateInfo framebufferInfo {};
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = &swapchain->GetImageViews()[i];
        framebufferInfo.width = swapchainInfo.m_extent.width;
        framebufferInfo.height = swapchainInfo.m_extent.height;
        framebufferInfo.layers = 1;

        out.emplace_back(deviceHandle.createFramebuffer(framebufferInfo));
    }

    return out;
}

void DestroyFramebuffers(vk::Device deviceHandle, std::vector<vk::Framebuffer>& framebuffers) {
    for (const auto& framebuffer : framebuffers) {
        deviceHandle.destroyFramebuffer(framebuffer);
    }
    framebuffers.clear();
}
}

CVulkanRenderer::CVulkanRenderer(const IVulkanWindow* const window) {
    if (window == nullptr) {
        throw std::runtime_error("Cannot initialize vulkan renderer. Window is nullptr");
    }

    m_context = std::make_unique<Vulkan::CRenderContext>(window);

    m_surface = std::make_unique<Vulkan::CSurface>(m_context.get());

    m_swapchain = std::make_unique<Vulkan::CSwapchain>(
        m_context.get(), m_surface->GetHandle(),
        3, vk::PresentModeKHR::eImmediate
    );

// #region RENDER_PASS
    vk::AttachmentDescription colorAttachment {};
    colorAttachment.format = m_swapchain->GetInfo().m_format.format;
    colorAttachment.samples = vk::SampleCountFlagBits::e1;
    colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
    colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
    colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    colorAttachment.initialLayout = vk::ImageLayout::eUndefined;
    colorAttachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;

    vk::AttachmentReference colorAttachmentRef {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

    vk::SubpassDescription subpass {};
    subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    vk::SubpassDependency dependency {};
    dependency.srcSubpass = vk::SubpassExternal;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dependency.srcAccessMask = {};
    dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;

    vk::RenderPassCreateInfo renderPassInfo {};
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;
    m_renderPass = m_context->GetDevice()->GetHandle().createRenderPass(renderPassInfo);
// #endregion RENDER_PASS

// #region SHADER_MODULES
    const std::vector<char> vertexShaderSource = ResourceSystem::LoadShader("shader.vert.spv");
    vk::ShaderModuleCreateInfo createInfo {};
    createInfo.codeSize = vertexShaderSource.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(vertexShaderSource.data());

    const vk::ShaderModule vertShaderModule = m_context->GetDevice()->GetHandle().createShaderModule(createInfo);

    const std::vector<char> fragmentShaderSource = ResourceSystem::LoadShader("shader.frag.spv");
    createInfo.codeSize = fragmentShaderSource.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(fragmentShaderSource.data());

    const vk::ShaderModule fragShaderModule = m_context->GetDevice()->GetHandle().createShaderModule(createInfo);

    vk::PipelineShaderStageCreateInfo vertShaderStageInfo {};
    vertShaderStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    vk::PipelineShaderStageCreateInfo fragShaderStageInfo {};
    fragShaderStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    vk::PipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };
// #endregion SHADER_MODULES

// #region PIPELINE
    vk::PipelineVertexInputStateCreateInfo vertexInputInfo {};
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;

    vk::PipelineInputAssemblyStateCreateInfo inputAssembly {};
    inputAssembly.topology = vk::PrimitiveTopology::eTriangleStrip;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    vk::PipelineViewportStateCreateInfo viewportState {};
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    vk::PipelineRasterizationStateCreateInfo rasterizer {};
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = vk::PolygonMode::eFill;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = vk::CullModeFlagBits::eNone;
    rasterizer.frontFace = vk::FrontFace::eClockwise;
    rasterizer.depthBiasEnable = VK_FALSE;

    vk::PipelineMultisampleStateCreateInfo multisampling {};
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;

    vk::PipelineColorBlendAttachmentState colorBlendAttachment {};
    colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
    colorBlendAttachment.blendEnable = VK_FALSE;

    vk::PipelineColorBlendStateCreateInfo colorBlending {};
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = vk::LogicOp::eCopy;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    std::vector dynamicStates = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor
    };
    vk::PipelineDynamicStateCreateInfo dynamicState {};
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    vk::PipelineLayoutCreateInfo pipelineLayoutInfo {};
    pipelineLayoutInfo.setLayoutCount = 0;
    pipelineLayoutInfo.pushConstantRangeCount = 0;

    m_pipelineLayout = m_context->GetDevice()->GetHandle().createPipelineLayout(pipelineLayoutInfo);

    vk::GraphicsPipelineCreateInfo pipelineInfo {};
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = m_pipelineLayout;
    pipelineInfo.renderPass = m_renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    m_pipeline = m_context->GetDevice()->GetHandle().createGraphicsPipeline(VK_NULL_HANDLE, pipelineInfo).value;
// #endregion

    m_context->GetDevice()->GetHandle().destroyShaderModule(vertShaderModule);
    m_context->GetDevice()->GetHandle().destroyShaderModule(fragShaderModule);

// #region COMMAND_BUFFERS
    vk::CommandPoolCreateInfo commandPoolInfo {};
    commandPoolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
    commandPoolInfo.queueFamilyIndex = m_context->GetDevice()->GetGraphicsQueue().m_familyIndex;

    m_commandPool = m_context->GetDevice()->GetHandle().createCommandPool(commandPoolInfo);

    m_commandBuffers.resize(m_maxFramesInFlight);
    vk::CommandBufferAllocateInfo cmdBufferAllocInfo {};
    cmdBufferAllocInfo.commandPool = m_commandPool;
    cmdBufferAllocInfo.level = vk::CommandBufferLevel::ePrimary;
    cmdBufferAllocInfo.commandBufferCount = static_cast<std::uint32_t>(m_commandBuffers.size());
    m_commandBuffers = m_context->GetDevice()->GetHandle().allocateCommandBuffers(cmdBufferAllocInfo);
// #endregion

    m_frameBuffers = CreateFrameBuffers(m_context->GetDevice()->GetHandle(), m_swapchain.get(), m_renderPass);

// #region SYNC_PRIMITIVES
    m_currentImageAvailableSemaphores.resize(m_maxFramesInFlight);
    m_renderFinishedSemaphores.resize(m_swapchain->GetInfo().m_imageCount);
    m_inFlightFences.resize(m_maxFramesInFlight);

    for (unsigned int i = 0; i < m_swapchain->GetInfo().m_imageCount; ++i) {
        m_renderFinishedSemaphores[i] = m_context->GetDevice()->GetHandle().createSemaphore(vk::SemaphoreCreateInfo {});
    }

    for (int i = 0; i < m_maxFramesInFlight; ++i) {
        m_currentImageAvailableSemaphores[i] = m_context->GetDevice()->GetHandle().createSemaphore(vk::SemaphoreCreateInfo {});
        m_inFlightFences[i] = m_context->GetDevice()->GetHandle().createFence(vk::FenceCreateInfo { vk::FenceCreateFlagBits::eSignaled });
    }
// #endregion SYNC_PRIMITIVES
}

std::unique_ptr<CVulkanRenderer> CVulkanRenderer::TryToCreate(const IVulkanWindow* const window) {
    try {
        return std::make_unique<CVulkanRenderer>(window);
    } catch (const std::exception& e) {
        Log::Error("Cannot initialize vulkan renderer: {}", e.what());
        return nullptr;
    }
}

void CVulkanRenderer::Draw() {
    auto deviceHandle = m_context->GetDevice()->GetHandle();
    auto swapchainHandle = m_swapchain->GetHandle();

// #region ACQUIRE_IMAGE
    std::ignore = deviceHandle.waitForFences(m_inFlightFences[m_frameIndex], vk::True, std::numeric_limits<unsigned int>::max());

    std::uint32_t imageIndex;
    auto res = deviceHandle.acquireNextImageKHR(
        swapchainHandle,
        std::numeric_limits<unsigned int>::max(),
        m_currentImageAvailableSemaphores[m_frameIndex],
        VK_NULL_HANDLE,
        &imageIndex
    );
    if (res == vk::Result::eErrorOutOfDateKHR) {
        m_swapchain->Recreate();
        DestroyFramebuffers(deviceHandle, m_frameBuffers);
        CreateFrameBuffers(deviceHandle, m_swapchain.get(), m_renderPass);
        return;
    }
    deviceHandle.resetFences(m_inFlightFences[m_frameIndex]);
    if (res != vk::Result::eSuccess && res != vk::Result::eSuboptimalKHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }
// #endregion ACQUIRE_IMAGE

// #region COMMAND_RECORD
    m_commandBuffers[m_frameIndex].reset();
    vk::CommandBufferBeginInfo beginInfo {};
    m_commandBuffers[m_frameIndex].begin(beginInfo);

    vk::RenderPassBeginInfo renderPassInfo {};
    renderPassInfo.renderPass = m_renderPass;
    renderPassInfo.framebuffer = m_frameBuffers[imageIndex];
    renderPassInfo.renderArea.offset = { { 0, 0 } };
    renderPassInfo.renderArea.extent = m_swapchain->GetInfo().m_extent;

    vk::ClearValue clearColor = vk::ClearColorValue { 0.0f, 0.0f, 0.005f, 1.0f };
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    m_commandBuffers[m_frameIndex].beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
    m_commandBuffers[m_frameIndex].bindPipeline(vk::PipelineBindPoint::eGraphics, m_pipeline);

    vk::Viewport viewport {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(m_swapchain->GetInfo().m_extent.width);
    viewport.height = static_cast<float>(m_swapchain->GetInfo().m_extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    m_commandBuffers[m_frameIndex].setViewport(0, 1, &viewport);

    vk::Rect2D scissor {};
    scissor.offset = { { 0, 0 } };
    scissor.extent = m_swapchain->GetInfo().m_extent;
    m_commandBuffers[m_frameIndex].setScissor(0, 1, &scissor);

    m_commandBuffers[m_frameIndex].draw(12, 1, 0, 0);

    m_commandBuffers[m_frameIndex].endRenderPass();
    m_commandBuffers[m_frameIndex].end();
// #endregion COMMAND_RECORD

// #region COMMAND_SUBMIT
    vk::SubmitInfo submitInfo {};

    vk::Semaphore waitSemaphores[] = { m_currentImageAvailableSemaphores[m_frameIndex] };
    vk::PipelineStageFlags waitStages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_commandBuffers[m_frameIndex];

    vk::Semaphore signalSemaphores[] = { m_renderFinishedSemaphores[imageIndex] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    std::ignore = m_context->GetDevice()->GetGraphicsQueue().m_handle.submit(1, &submitInfo, m_inFlightFences[m_frameIndex]);
// #endregion COMMAND_RECORD

// #region PRESENT
    vk::PresentInfoKHR presentInfo {};

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchainHandle;

    presentInfo.pImageIndices = &imageIndex;

    res = m_context->GetDevice()->GetPresentQueue().m_handle.presentKHR(&presentInfo);
    if (res == vk::Result::eErrorOutOfDateKHR) {
        m_swapchain->Recreate();
        DestroyFramebuffers(deviceHandle, m_frameBuffers);
        m_frameBuffers = CreateFrameBuffers(deviceHandle, m_swapchain.get(), m_renderPass);
        return;
    }
// #endregion PRESENT

    m_frameIndex = (m_frameIndex + 1) % m_maxFramesInFlight;
}

CVulkanRenderer::~CVulkanRenderer() {
    m_context->GetDevice()->GetHandle().waitIdle();
    for (auto ias : m_currentImageAvailableSemaphores) {
        m_context->GetDevice()->GetHandle().destroySemaphore(ias);
    }
    for (auto iff : m_inFlightFences) {
        m_context->GetDevice()->GetHandle().destroyFence(iff);
    }
    for (auto rfs : m_renderFinishedSemaphores) {
        m_context->GetDevice()->GetHandle().destroySemaphore(rfs);
    }
    m_context->GetDevice()->GetHandle().destroyCommandPool(m_commandPool);
    for (auto framebuffer : m_frameBuffers) {
        m_context->GetDevice()->GetHandle().destroyFramebuffer(framebuffer);
    }
    m_context->GetDevice()->GetHandle().destroyPipeline(m_pipeline);
    m_context->GetDevice()->GetHandle().destroyPipelineLayout(m_pipelineLayout);
    m_context->GetDevice()->GetHandle().destroyRenderPass(m_renderPass);
}
