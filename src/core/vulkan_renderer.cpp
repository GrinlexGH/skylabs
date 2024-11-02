#include "console.hpp"
#include "vulkan_renderer.hpp"
#include "SDL.hpp"
#include "SDL_Vulkan.hpp"
#include "unicode.hpp"
#include "resourceloader.hpp"

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#include "vk_mem_alloc.hpp"

#include "stb_image.h"

#include <fstream>
#include <filesystem>
#include <chrono>
#include <cassert>
#include <set>

#ifdef NDEBUG
bool g_enableValidationLayers = false;
#else
bool g_enableValidationLayers = true;
#endif

constexpr std::size_t MAX_FRAMES_IN_FLIGHT = 1;

const std::vector<CVertex> g_vertices = {
    {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
    {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
    {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
    {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}
};

const std::vector<uint16_t> g_indices = {
    0, 1, 2, 2, 3, 0
};

//============
// CVulkanRenderer

CVulkanRenderer::CVulkanRenderer(IWindow* window) {
    m_context.Initialize(window);

    RegisterRequestedDeviceExtensions();
    m_physicalDevice = PickPhysicalDevice();

    m_queueFamilies = GetQueueFamilies(m_physicalDevice);
    InitializeDeviceExtensions(m_physicalDevice, m_enabledDeviceExtensions);

    InitializeLogicalDevice();

    CreateAllocator();

    m_graphicsQueue = m_device.getQueue(m_queueFamilies.m_graphicsFamily.value(), 0);
    m_presentQueue = m_device.getQueue(m_queueFamilies.m_presentFamily.value(), 0);

    m_swapChainInfo = GetSwapChainInfo(m_physicalDevice);
    m_surfaceFormat = ChoosetSurfaceFormat(m_swapChainInfo.m_formats);
    m_presentMode = ChoosePresentMode(m_swapChainInfo.m_presentModes);
    m_swapChainExtent = ChooseSwapChainExtent(m_swapChainInfo.m_capabilities);
    CreateSwapChain(m_surfaceFormat, m_presentMode, m_swapChainExtent);

    m_images = m_device.getSwapchainImagesKHR(m_swapChain);
    CreateImageViews(m_images, m_surfaceFormat.format);

    CreateRenderPass();

    m_descriptorSetLayout = CreateDescriptorSetLayout();
    CreatePipeline();

    CreateFramebuffers();

    CreateCommandPool();
    CreateCommandBuffers();

    CreateVertexBuffer();
    CreateIndexBuffer();

    CreateUniformBuffers();

    CreateTextureImage();
    CreateTextureImageView(m_surfaceFormat.format);
    CreateTextureSampler();

    CreateDescriptorPool();
    CreateDescriptorSets();

    m_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    for (std::size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        m_imageAvailableSemaphores[i] = m_device.createSemaphore(vk::SemaphoreCreateInfo {});
        m_renderFinishedSemaphores[i] = m_device.createSemaphore(vk::SemaphoreCreateInfo {});
        m_inFlightFences[i] = m_device.createFence(vk::FenceCreateInfo { vk::FenceCreateFlagBits::eSignaled });
    }
}

CVulkanRenderer::~CVulkanRenderer() {
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        m_device.destroySemaphore(m_imageAvailableSemaphores[i]);
        m_device.destroySemaphore(m_renderFinishedSemaphores[i]);
        m_device.destroyFence(m_inFlightFences[i]);
    }

    m_device.destroySampler(m_textureSampler);
    m_device.destroyImageView(m_textureImageView);
    m_allocator.destroyImage(m_textureImage.image, m_textureImage.allocation);

    m_device.destroyDescriptorPool(m_descriptorPool);
    m_device.destroyDescriptorSetLayout(m_descriptorSetLayout);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        m_allocator.unmapMemory(m_uniformBuffers[i].allocation);
        m_allocator.destroyBuffer(m_uniformBuffers[i].buffer, m_uniformBuffers[i].allocation);
    }

    m_allocator.destroyBuffer(m_indexBuffer.buffer, m_indexBuffer.allocation);
    m_allocator.destroyBuffer(m_vertexBuffer.buffer, m_vertexBuffer.allocation);

    CleanupSwapChain();

    m_context.GetInstance().destroySurfaceKHR(m_context.GetSurface());

    m_device.destroyCommandPool(m_commandPool);

    m_device.destroyPipeline(m_pipeline);
    m_device.destroyPipelineLayout(m_pipelineLayout);

    m_device.destroyRenderPass(m_renderPass);

    vmaDestroyAllocator(m_allocator);
    m_device.destroy();

    if (g_enableValidationLayers) {
        m_context.GetInstance().destroyDebugUtilsMessengerEXT(m_context.GetDebugMessenger());
    }

    m_context.GetInstance().destroy();
}

void CVulkanRenderer::CreateAllocator() {
    vma::AllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.flags = vma::AllocatorCreateFlagBits::eExtMemoryBudget;
    allocatorInfo.vulkanApiVersion = vk::ApiVersion13;
    allocatorInfo.physicalDevice = m_physicalDevice;
    allocatorInfo.device = m_device;
    allocatorInfo.instance = m_context.GetInstance();

    vma::VulkanFunctions vulkanFunctions = vma::functionsFromDispatcher();
    allocatorInfo.pVulkanFunctions = &vulkanFunctions;

    m_allocator = vma::createAllocator(allocatorInfo);
}

CVulkanRenderer::CQueueFamilyIndices CVulkanRenderer::GetQueueFamilies(vk::PhysicalDevice physicalDevice) {
    std::vector<vk::QueueFamilyProperties> queueFamiliesProperties = physicalDevice.getQueueFamilyProperties();

    CQueueFamilyIndices queueFamilies {};

    for (std::size_t i = 0; i < queueFamiliesProperties.size(); ++i) {
        if (queueFamiliesProperties[i].queueFlags & vk::QueueFlagBits::eGraphics) {
            queueFamilies.m_graphicsFamily = static_cast<uint32_t>(i);
        }
        if (physicalDevice.getSurfaceSupportKHR(static_cast<uint32_t>(i), m_context.GetSurface())) {
            queueFamilies.m_presentFamily = static_cast<uint32_t>(i);
        }
        if (queueFamilies.isComplete()) {
            break;
        }
    }

    return queueFamilies;
}

void CVulkanRenderer::RegisterRequestedDeviceExtensions() {
    m_requestedDeviceExtensions[VK_KHR_SWAPCHAIN_EXTENSION_NAME] = true;
    m_requestedDeviceExtensions[VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME] = true;
}

bool CVulkanRenderer::InitializeDeviceExtensions(
    vk::PhysicalDevice physicalDevice,
    std::unordered_set<std::string>& enabledExtensions
) {
    // Enable all requested extensions that are supported
    std::vector<vk::ExtensionProperties> deviceExtensions = physicalDevice.enumerateDeviceExtensionProperties();
    for (uint32_t i = 0; i < deviceExtensions.size(); ++i) {
        if (m_requestedDeviceExtensions.contains(deviceExtensions[i].extensionName)) {
            enabledExtensions.insert(deviceExtensions[i].extensionName);
        }
    }

    // Check for required extensions
    // .first is extension name, .second is required or not
    bool allReqExtsFound = true;
    for (const std::pair<const std::string, bool>& reqExt : m_requestedDeviceExtensions) {
        if (std::ranges::find(enabledExtensions, reqExt.first.c_str()) == enabledExtensions.end()) {
            if (reqExt.second) {
                Error("Required instance extension \"%s\" not found.\n", reqExt.first.c_str());
                allReqExtsFound = false;
            } else {
                Msg("Optional instance extension \"%s\" not found.\n", reqExt.first.c_str());
            }
        }
    }

    return allReqExtsFound;
}

CVulkanRenderer::CSwapChainInfo CVulkanRenderer::GetSwapChainInfo(vk::PhysicalDevice physicalDevice) {
    CSwapChainInfo swapChainInfo {};

    swapChainInfo.m_capabilities = physicalDevice.getSurfaceCapabilitiesKHR(m_context.GetSurface());
    swapChainInfo.m_formats = physicalDevice.getSurfaceFormatsKHR(m_context.GetSurface());
    swapChainInfo.m_presentModes = physicalDevice.getSurfacePresentModesKHR(m_context.GetSurface());

    return swapChainInfo;
}

bool CVulkanRenderer::isDeviceSuitable(vk::PhysicalDevice physicalDevice) {
    if (!GetQueueFamilies(physicalDevice).isComplete())
        return false;

    std::unordered_set<std::string> stub;
    if (!InitializeDeviceExtensions(physicalDevice, stub))
        return false;

    CSwapChainInfo swapChainInfo = GetSwapChainInfo(physicalDevice);
    if (swapChainInfo.m_formats.empty() || swapChainInfo.m_presentModes.empty())
        return false;

    vk::PhysicalDeviceFeatures supportedFeatures = physicalDevice.getFeatures();;
    if (!supportedFeatures.fillModeNonSolid || !supportedFeatures.samplerAnisotropy)
        return false;

    return true;
}

vk::PhysicalDevice CVulkanRenderer::PickPhysicalDevice() {
    std::vector<vk::PhysicalDevice> devices = m_context.GetInstance().enumeratePhysicalDevices();
    if (devices.size() == 0) {
        throw std::runtime_error("Failed to find GPUs with Vulkan support!\n");
    }

    // Choose the device
    vk::PhysicalDevice physicalDevice = VK_NULL_HANDLE;
    for (const vk::PhysicalDevice& device : devices) {
        if (isDeviceSuitable(device))
            physicalDevice = device;
    }

    if (physicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("Failed to find a suitable GPU!\n");
    }

    m_physicslDeviceProps = physicalDevice.getProperties();

    return physicalDevice;
}

void CVulkanRenderer::InitializeLogicalDevice() {
    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos {};
    std::set<uint32_t> uniqueQueueFamilies = {
        m_queueFamilies.m_graphicsFamily.value(),
        m_queueFamilies.m_presentFamily.value()
    };

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        vk::DeviceQueueCreateInfo queueCreateInfo {};
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    std::vector<const char*> enabledExtensions {};
    enabledExtensions.reserve(m_enabledDeviceExtensions.size());
    for (const std::string& extension_name : m_enabledDeviceExtensions) {
        enabledExtensions.push_back(extension_name.c_str());
    }

    m_requestedDeviceFeatures.samplerAnisotropy = true;

    vk::DeviceCreateInfo deviceInfo {};
    deviceInfo.pQueueCreateInfos = queueCreateInfos.data();
    deviceInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    deviceInfo.pEnabledFeatures = &m_requestedDeviceFeatures;
    deviceInfo.enabledExtensionCount = static_cast<uint32_t>(enabledExtensions.size());
    deviceInfo.ppEnabledExtensionNames = enabledExtensions.data();

    m_device = m_physicalDevice.createDevice(deviceInfo);
}

vk::SurfaceFormatKHR CVulkanRenderer::ChoosetSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& formats) {
    for (const auto& availableFormat : m_swapChainInfo.m_formats) {
        if (availableFormat.format == vk::Format::eR8G8B8A8Srgb &&
            availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear
        ) {
            return availableFormat;
        }
    }

    Error << "Surface format was not selected!\n";
    return formats[0];
}

vk::PresentModeKHR CVulkanRenderer::ChoosePresentMode(const std::vector<vk::PresentModeKHR>& presentModes) {
    for (const auto& availablePresentMode : presentModes) {
        // VSYNC
        if (availablePresentMode == vk::PresentModeKHR::eImmediate) {
            return availablePresentMode;
        }
    }

    Error << "Correct present mode was not selected!\n";
    return presentModes[0];
}

vk::Extent2D CVulkanRenderer::ChooseSwapChainExtent(const vk::SurfaceCapabilitiesKHR& capabilities) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    } else {
        int width = 0, height = 0;
        if (m_context.GetWindow()->GetVendor() == WindowVendor::eSDL) {    // TODO: make special vulkan window
            SDL::Vulkan::GetDrawableSize(
                static_cast<SDL_Window*>(m_context.GetWindow()->GetHandle()),
                &width,
                &height
            );
        } else {
            assert(false);
        }

        vk::Extent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        actualExtent.width = std::clamp(
            actualExtent.width,
            m_swapChainInfo.m_capabilities.minImageExtent.width,
            m_swapChainInfo.m_capabilities.maxImageExtent.width
        );
        actualExtent.height = std::clamp(
            actualExtent.height,
            m_swapChainInfo.m_capabilities.minImageExtent.height,
            m_swapChainInfo.m_capabilities.maxImageExtent.height
        );

        return actualExtent;
    }
}

void CVulkanRenderer::CreateSwapChain(
    vk::SurfaceFormatKHR surfaceFormat,
    vk::PresentModeKHR presentMode,
    vk::Extent2D extent
) {
    uint32_t imageCount = m_swapChainInfo.m_capabilities.minImageCount;

    if (m_swapChainInfo.m_capabilities.maxImageCount > 0 && imageCount > m_swapChainInfo.m_capabilities.maxImageCount) {
        imageCount = m_swapChainInfo.m_capabilities.maxImageCount;
    }

    vk::SwapchainCreateInfoKHR swapChainInfo {};
    swapChainInfo.surface = m_context.GetSurface();
    swapChainInfo.minImageCount = imageCount;
    swapChainInfo.imageFormat = surfaceFormat.format;
    swapChainInfo.imageColorSpace = surfaceFormat.colorSpace;
    swapChainInfo.imageExtent = extent;
    swapChainInfo.imageArrayLayers = 1;
    swapChainInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;

    uint32_t queueFamilyIndices[] = {
        m_queueFamilies.m_graphicsFamily.value(),
        m_queueFamilies.m_presentFamily.value()
    };

    if (m_queueFamilies.m_graphicsFamily != m_queueFamilies.m_presentFamily) {
        swapChainInfo.imageSharingMode = vk::SharingMode::eConcurrent;
        swapChainInfo.queueFamilyIndexCount = 2;
        swapChainInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        swapChainInfo.imageSharingMode = vk::SharingMode::eExclusive;
    }

    swapChainInfo.preTransform = m_swapChainInfo.m_capabilities.currentTransform;
    swapChainInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    swapChainInfo.presentMode = presentMode;
    swapChainInfo.clipped = vk::False;

    m_swapChain = m_device.createSwapchainKHR(swapChainInfo);
}

vk::ImageView CVulkanRenderer::CreateImageView(vk::Image image, vk::Format format) {
    vk::ImageViewCreateInfo imageViewInfo {};
    imageViewInfo.image = image;
    imageViewInfo.viewType = vk::ImageViewType::e2D;
    imageViewInfo.format = format;
    imageViewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    imageViewInfo.subresourceRange.baseMipLevel = 0;
    imageViewInfo.subresourceRange.levelCount = 1;
    imageViewInfo.subresourceRange.baseArrayLayer = 0;
    imageViewInfo.subresourceRange.layerCount = 1;

    return m_device.createImageView(imageViewInfo);
}

void CVulkanRenderer::CreateImageViews(const std::vector<vk::Image>& images, vk::Format format) {
    m_imageViews.resize(images.size());

    for (std::size_t i = 0; i < images.size(); ++i) {
        m_imageViews[i] = CreateImageView(images[i], format);
    }
}

void CVulkanRenderer::CreateRenderPass() {
    vk::AttachmentDescription colorAttachment {};
    colorAttachment.format = m_surfaceFormat.format;
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

    vk::SubpassDescription subpassDesc {};
    subpassDesc.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
    subpassDesc.colorAttachmentCount = 1;
    subpassDesc.pColorAttachments = &colorAttachmentRef;

    vk::SubpassDependency dependency {};
    dependency.srcSubpass = vk::SubpassExternal;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dependency.srcAccessMask = vk::AccessFlagBits::eNone;
    dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;

    vk::RenderPassCreateInfo renderPassInfo {};
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpassDesc;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    m_renderPass = m_device.createRenderPass(renderPassInfo);
}

vk::DescriptorSetLayout CVulkanRenderer::CreateDescriptorSetLayout() {
    vk::DescriptorSetLayoutBinding uboLayoutBinding {};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eVertex;
    uboLayoutBinding.pImmutableSamplers = nullptr;

    vk::DescriptorSetLayoutBinding samplerLayoutBinding {};
    samplerLayoutBinding.binding = 1;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    samplerLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;

    std::array<vk::DescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };
    vk::DescriptorSetLayoutCreateInfo layoutInfo {};
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    return m_device.createDescriptorSetLayout(layoutInfo);
}

vk::ShaderModule CVulkanRenderer::CreateShaderModule(const std::vector<char>& byteCode) {
    vk::ShaderModuleCreateInfo shaderModuleInfo {};
    shaderModuleInfo.codeSize = byteCode.size();
    shaderModuleInfo.pCode = reinterpret_cast<const uint32_t*>(byteCode.data());

    vk::ShaderModule shaderModule = m_device.createShaderModule(shaderModuleInfo);
    return shaderModule;
}

void CVulkanRenderer::CreatePipeline() {
    vk::GraphicsPipelineCreateInfo pipelineInfo {};

    auto vertShaderCode = resource_loader::ReadFile("shaders/vert.spv");
    auto fragShaderCode = resource_loader::ReadFile("shaders/frag.spv");

    vk::ShaderModule vertexShader = CreateShaderModule(vertShaderCode);
    vk::ShaderModule fragmentShader = CreateShaderModule(fragShaderCode);

    vk::PipelineShaderStageCreateInfo vertShaderStageInfo {};
    vertShaderStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
    vertShaderStageInfo.module = vertexShader;
    vertShaderStageInfo.pName = "main";
    vk::PipelineShaderStageCreateInfo fragShaderStageInfo {};
    fragShaderStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
    fragShaderStageInfo.module = fragmentShader;
    fragShaderStageInfo.pName = "main";

    vk::PipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;

    //==========
    vk::VertexInputBindingDescription bindingDescription = CVertex::getBindingDescription();
    std::array<vk::VertexInputAttributeDescription, 3> attributeDescriptions = CVertex::getAttributeDescriptions();

    vk::PipelineVertexInputStateCreateInfo vertexInputInfo {};
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = attributeDescriptions.size();
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    pipelineInfo.pVertexInputState = &vertexInputInfo;

    //==========
    vk::PipelineInputAssemblyStateCreateInfo inputAssembly {};
    inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;
    inputAssembly.primitiveRestartEnable = vk::False;

    pipelineInfo.pInputAssemblyState = &inputAssembly;

    //==========
    vk::PipelineViewportStateCreateInfo viewportState {};
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    pipelineInfo.pViewportState = &viewportState;

    //==========
    std::vector<vk::DynamicState> dynamicStates = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor
    };

    vk::PipelineDynamicStateCreateInfo dynamicState {};
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    pipelineInfo.pDynamicState = &dynamicState;

    //==========
    vk::PipelineRasterizationStateCreateInfo rasterizer {};
    rasterizer.depthClampEnable = vk::False;
    rasterizer.rasterizerDiscardEnable = vk::False;
    rasterizer.polygonMode = vk::PolygonMode::eFill;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = vk::CullModeFlagBits::eBack;
    rasterizer.frontFace = vk::FrontFace::eCounterClockwise;
    rasterizer.depthBiasEnable = vk::False;

    pipelineInfo.pRasterizationState = &rasterizer;

    //==========
    vk::PipelineMultisampleStateCreateInfo multisampling {};
    multisampling.sampleShadingEnable = vk::False;
    multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;

    pipelineInfo.pMultisampleState = &multisampling;

    //==========
    vk::PipelineColorBlendAttachmentState colorBlendAttachment {};
    colorBlendAttachment.colorWriteMask =
        vk::ColorComponentFlagBits::eR |
        vk::ColorComponentFlagBits::eG |
        vk::ColorComponentFlagBits::eB |
        vk::ColorComponentFlagBits::eA;
    colorBlendAttachment.blendEnable = vk::True;
    colorBlendAttachment.srcColorBlendFactor = vk::BlendFactor::eOne;
    colorBlendAttachment.dstColorBlendFactor = vk::BlendFactor::eZero;
    colorBlendAttachment.colorBlendOp = vk::BlendOp::eAdd;
    colorBlendAttachment.srcAlphaBlendFactor = vk::BlendFactor::eOne;
    colorBlendAttachment.dstAlphaBlendFactor = vk::BlendFactor::eZero;
    colorBlendAttachment.alphaBlendOp = vk::BlendOp::eAdd;

    vk::PipelineColorBlendStateCreateInfo colorBlending {};
    colorBlending.logicOpEnable = vk::False;
    colorBlending.logicOp = vk::LogicOp::eCopy;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 1.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    pipelineInfo.pColorBlendState = &colorBlending;

    //==========
    vk::PipelineLayoutCreateInfo pipelineLayoutInfo {};
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &m_descriptorSetLayout;

    m_pipelineLayout = m_device.createPipelineLayout(pipelineLayoutInfo);

    //==========
    pipelineInfo.layout = m_pipelineLayout;
    pipelineInfo.renderPass = m_renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;

    m_pipeline = m_device.createGraphicsPipeline(VK_NULL_HANDLE, pipelineInfo).value;

    m_device.destroyShaderModule(fragmentShader);
    m_device.destroyShaderModule(vertexShader);
}

void CVulkanRenderer::CreateFramebuffers() {
    m_frameBuffers.resize(m_imageViews.size());

    for (std::size_t i = 0; i < m_imageViews.size(); i++) {
        vk::ImageView attachments[] = {
            m_imageViews[i]
        };
        vk::FramebufferCreateInfo frameBufferInfo {};
        frameBufferInfo.renderPass = m_renderPass;
        frameBufferInfo.attachmentCount = 1;
        frameBufferInfo.pAttachments = attachments;
        frameBufferInfo.width = m_swapChainExtent.width;
        frameBufferInfo.height = m_swapChainExtent.height;
        frameBufferInfo.layers = 1;

        m_frameBuffers[i] = m_device.createFramebuffer(frameBufferInfo);
    }
}

void CVulkanRenderer::CreateCommandPool() {
    vk::CommandPoolCreateInfo commandPoolInfo {};
    commandPoolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
    commandPoolInfo.queueFamilyIndex = m_queueFamilies.m_graphicsFamily.value();

    m_commandPool = m_device.createCommandPool(commandPoolInfo);
}

void CVulkanRenderer::CreateCommandBuffers() {
    m_commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    vk::CommandBufferAllocateInfo cmdBufferAllocInfo {};
    cmdBufferAllocInfo.commandPool = m_commandPool;
    cmdBufferAllocInfo.level = vk::CommandBufferLevel::ePrimary;
    cmdBufferAllocInfo.commandBufferCount = static_cast<uint32_t>(m_commandBuffers.size());

    m_commandBuffers = m_device.allocateCommandBuffers(cmdBufferAllocInfo);
}

CVulkanRenderer::CBuffer CVulkanRenderer::CreateBuffer(
    vk::DeviceSize size,
    vk::BufferUsageFlags usage,
    vk::MemoryPropertyFlags properties,
    vma::AllocationCreateFlags flags
) {
    CBuffer result {};

    vk::BufferCreateInfo bufferInfo {};
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = vk::SharingMode::eExclusive;

    vma::AllocationCreateInfo allocInfo = {};
    allocInfo.usage = vma::MemoryUsage::eAuto;
    allocInfo.requiredFlags = properties;
    allocInfo.flags = flags;

    std::pair<vk::Buffer, vma::Allocation> buffer = m_allocator.createBuffer(bufferInfo, allocInfo);
    result.buffer = buffer.first;
    result.allocation = buffer.second;

    return result;
}

vk::CommandBuffer CVulkanRenderer::BeginSingleTimeCommands() {
    vk::CommandBufferAllocateInfo allocInfo {};
    allocInfo.level = vk::CommandBufferLevel::ePrimary;
    allocInfo.commandPool = m_commandPool;
    allocInfo.commandBufferCount = 1;

    vk::CommandBuffer commandBuffer = m_device.allocateCommandBuffers(allocInfo)[0];

    vk::CommandBufferBeginInfo beginInfo {};
    beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
    commandBuffer.begin(beginInfo);

    return commandBuffer;
}

void CVulkanRenderer::EndSingleTimeCommands(vk::CommandBuffer commandBuffer) {
    commandBuffer.end();

    vk::SubmitInfo submitInfo {};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    m_graphicsQueue.submit(submitInfo);
    m_graphicsQueue.waitIdle();

    m_device.freeCommandBuffers(m_commandPool, commandBuffer);
}

void CVulkanRenderer::CopyBuffer(
    vk::Buffer srcBuffer,
    vk::Buffer dstBuffer,
    vk::DeviceSize size
) {
    vk::CommandBuffer commandBuffer = BeginSingleTimeCommands();

    vk::BufferCopy copyRegion {};
    copyRegion.size = size;
    commandBuffer.copyBuffer(srcBuffer, dstBuffer, copyRegion);

    EndSingleTimeCommands(commandBuffer);
}

void CVulkanRenderer::CreateVertexBuffer() {
    vk::DeviceSize bufferSize = sizeof(g_vertices[0]) * g_vertices.size();

    CBuffer stagingBuffer = CreateBuffer(
        bufferSize,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
        vma::AllocationCreateFlagBits::eHostAccessRandom
    );

    stagingBuffer.mapped = m_allocator.mapMemory(stagingBuffer.allocation);
        memcpy(stagingBuffer.mapped, g_vertices.data(), (std::size_t)bufferSize);
    m_allocator.unmapMemory(stagingBuffer.allocation);

    m_vertexBuffer = CreateBuffer(
        bufferSize,
        vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        {}
    );

    CopyBuffer(stagingBuffer.buffer, m_vertexBuffer.buffer, bufferSize);

    m_allocator.destroyBuffer(stagingBuffer.buffer, stagingBuffer.allocation);
}

void CVulkanRenderer::CreateIndexBuffer() {
    vk::DeviceSize bufferSize = sizeof(g_indices[0]) * g_indices.size();

    CBuffer stagingBuffer = CreateBuffer(
        bufferSize,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
        vma::AllocationCreateFlagBits::eHostAccessRandom
    );

    stagingBuffer.mapped = m_allocator.mapMemory(stagingBuffer.allocation);
        memcpy(stagingBuffer.mapped, g_indices.data(), (std::size_t)bufferSize);
    m_allocator.unmapMemory(stagingBuffer.allocation);

    m_indexBuffer = CreateBuffer(
        bufferSize,
        vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        {}
    );

    CopyBuffer(stagingBuffer.buffer, m_indexBuffer.buffer, bufferSize);

    m_allocator.destroyBuffer(stagingBuffer.buffer, stagingBuffer.allocation);
}

void CVulkanRenderer::CreateUniformBuffers() {
    vk::DeviceSize bufferSize = sizeof(CUniformBufferObject);

    m_uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    for (std::size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        m_uniformBuffers[i] = CreateBuffer(
            bufferSize,
            vk::BufferUsageFlagBits::eUniformBuffer,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
            vma::AllocationCreateFlagBits::eHostAccessRandom
        );
        m_uniformBuffers[i].mapped = m_allocator.mapMemory(m_uniformBuffers[i].allocation);
    }
}

void CVulkanRenderer::CreateDescriptorPool() {
    std::array<vk::DescriptorPoolSize, 2> poolSizes{};
    poolSizes[0].type = vk::DescriptorType::eUniformBuffer;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    poolSizes[1].type = vk::DescriptorType::eCombinedImageSampler;
    poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    vk::DescriptorPoolCreateInfo poolInfo {};
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    m_descriptorPool = m_device.createDescriptorPool(poolInfo);
}

void CVulkanRenderer::CreateDescriptorSets() {
    std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, m_descriptorSetLayout);

    vk::DescriptorSetAllocateInfo allocInfo {};
    allocInfo.descriptorPool = m_descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    allocInfo.pSetLayouts = layouts.data();

    m_descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    m_descriptorSets = m_device.allocateDescriptorSets(allocInfo);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vk::DescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = m_uniformBuffers[i].buffer;
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(CUniformBufferObject);

        vk::DescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        imageInfo.imageView = m_textureImageView;
        imageInfo.sampler = m_textureSampler;

        std::array<vk::WriteDescriptorSet, 2> descriptorWrites {};

        descriptorWrites[0].dstSet = m_descriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = vk::DescriptorType::eUniformBuffer;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfo;

        descriptorWrites[1].dstSet = m_descriptorSets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = vk::DescriptorType::eCombinedImageSampler;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pImageInfo = &imageInfo;

        m_device.updateDescriptorSets(static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }
}

CVulkanRenderer::CImage CVulkanRenderer::CreateImage(
    uint32_t width,
    uint32_t height,
    vk::Format format,
    vk::ImageTiling tiling,
    vk::ImageUsageFlags usage,
    vk::MemoryPropertyFlags properties
) {
    CImage result {};

    vk::ImageCreateInfo imageInfo{};
    imageInfo.imageType = vk::ImageType::e2D;
    imageInfo.extent.width = static_cast<uint32_t>(width);
    imageInfo.extent.height = static_cast<uint32_t>(height);
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = vk::ImageLayout::eUndefined;
    imageInfo.usage = usage;
    imageInfo.sharingMode = vk::SharingMode::eExclusive;
    imageInfo.samples = vk::SampleCountFlagBits::e1;

    vma::AllocationCreateInfo allocInfo {};
    allocInfo.usage = vma::MemoryUsage::eAuto;
    allocInfo.requiredFlags = properties;

    std::pair<vk::Image, vma::Allocation> image = m_allocator.createImage(imageInfo, allocInfo);
    result.image = image.first;
    result.allocation = image.second;

    return result;
}

void CVulkanRenderer::TransitionImageLayout(
    vk::Image image,
    vk::Format format,
    vk::ImageLayout oldLayout,
    vk::ImageLayout newLayout
) {
    (void)format;
    vk::CommandBuffer commandBuffer = BeginSingleTimeCommands();

    vk::ImageMemoryBarrier barrier {};
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
    barrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    vk::PipelineStageFlags sourceStage {};
    vk::PipelineStageFlags destinationStage {};

    if (
        oldLayout == vk::ImageLayout::eUndefined &&
        newLayout == vk::ImageLayout::eTransferDstOptimal
    ) {
        barrier.srcAccessMask = {};
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

        sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
        destinationStage = vk::PipelineStageFlagBits::eTransfer;
    } else if (
        oldLayout == vk::ImageLayout::eTransferDstOptimal &&
        newLayout == vk::ImageLayout::eShaderReadOnlyOptimal
    ) {
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

        sourceStage = vk::PipelineStageFlagBits::eTransfer;
        destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
    } else {
        throw std::invalid_argument("unsupported layout transition!");
    }

    commandBuffer.pipelineBarrier(
        sourceStage, destinationStage,
        {},
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    EndSingleTimeCommands(commandBuffer);
}

void CVulkanRenderer::CopyBufferToImage(
    vk::Buffer buffer,
    vk::Image image,
    uint32_t width,
    uint32_t height
) {
    vk::CommandBuffer commandBuffer = BeginSingleTimeCommands();

    vk::BufferImageCopy region {};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;

    region.imageOffset = vk::Offset3D {0, 0, 0};
    region.imageExtent = vk::Extent3D {
        width,
        height,
        1
    };

    commandBuffer.copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, 1, &region);

    EndSingleTimeCommands(commandBuffer);
}

void CVulkanRenderer::CreateTextureImage() {
    int texWidth = 0, texHeight = 0, texChannels = 0;
    stbi_uc* pixels = stbi_load("texture.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    if (!pixels) {
        throw std::runtime_error("failed to load texture image!");
    }

    vk::DeviceSize imageSize = texWidth * texHeight * 4;

    CBuffer stagingBuffer = CreateBuffer(
        imageSize,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
        vma::AllocationCreateFlagBits::eHostAccessRandom
    );

    stagingBuffer.mapped = m_allocator.mapMemory(stagingBuffer.allocation);
        memcpy(stagingBuffer.mapped, pixels, static_cast<size_t>(imageSize));
    m_allocator.unmapMemory(stagingBuffer.allocation);

    stbi_image_free(pixels);

    m_textureImage = CreateImage(
        texWidth,
        texHeight,
        vk::Format::eR8G8B8A8Srgb,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
        vk::MemoryPropertyFlagBits::eDeviceLocal
    );

    TransitionImageLayout(
        m_textureImage.image,
        vk::Format::eR8G8B8A8Srgb,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eTransferDstOptimal
    );

    CopyBufferToImage(
        stagingBuffer.buffer,
        m_textureImage.image,
        static_cast<uint32_t>(texWidth),
        static_cast<uint32_t>(texHeight)
    );

    TransitionImageLayout(
        m_textureImage.image,
        vk::Format::eR8G8B8A8Srgb,
        vk::ImageLayout::eTransferDstOptimal,
        vk::ImageLayout::eShaderReadOnlyOptimal
    );

    m_allocator.destroyBuffer(stagingBuffer.buffer, stagingBuffer.allocation);
}

void CVulkanRenderer::CreateTextureImageView(vk::Format format) {
    m_textureImageView = CreateImageView(m_textureImage.image, format);
}

void CVulkanRenderer::CreateTextureSampler() {
    vk::SamplerCreateInfo samplerInfo {};
    samplerInfo.magFilter = vk::Filter::eLinear;
    samplerInfo.minFilter = vk::Filter::eLinear;
    samplerInfo.addressModeU = vk::SamplerAddressMode::eRepeat;
    samplerInfo.addressModeV = vk::SamplerAddressMode::eRepeat;
    samplerInfo.addressModeW = vk::SamplerAddressMode::eRepeat;
    samplerInfo.anisotropyEnable = vk::True;
    samplerInfo.maxAnisotropy = m_physicslDeviceProps.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = vk::BorderColor::eIntOpaqueBlack;
    samplerInfo.unnormalizedCoordinates = vk::False;
    samplerInfo.compareEnable = vk::False;
    samplerInfo.compareOp = vk::CompareOp::eAlways;
    samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    m_textureSampler = m_device.createSampler(samplerInfo);
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

    UpdateUniformBuffer(m_currentFrame, m_swapChainExtent);

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
    clearColor.color = std::array<float, 4> { 0.0f, 0.0f, 0.005f, 1.0f };
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;
    m_commandBuffers[m_currentFrame].beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
    m_commandBuffers[m_currentFrame].bindPipeline(vk::PipelineBindPoint::eGraphics, m_pipeline);

    vk::Buffer vertexBuffers[] = { m_vertexBuffer.buffer };
    VkDeviceSize offsets[] = { 0 };
    m_commandBuffers[m_currentFrame].bindVertexBuffers(0, 1, vertexBuffers, offsets);
    m_commandBuffers[m_currentFrame].bindIndexBuffer(m_indexBuffer.buffer, 0, vk::IndexType::eUint16);

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

    m_commandBuffers[m_currentFrame].bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics,
        m_pipelineLayout,
        0,
        1,
        &m_descriptorSets[m_currentFrame],
        0,
        nullptr
    );
    m_commandBuffers[m_currentFrame].drawIndexed(
        static_cast<uint32_t>(g_indices.size()),
        1, 0, 0, 0
    );

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
    m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    m_device.waitIdle();
}

void CVulkanRenderer::RecreateSwapChain() {
    m_device.waitIdle();

    CleanupSwapChain();

    m_swapChainInfo = GetSwapChainInfo(m_physicalDevice);

    m_surfaceFormat = ChoosetSurfaceFormat(m_swapChainInfo.m_formats);
    m_presentMode = ChoosePresentMode(m_swapChainInfo.m_presentModes);
    m_swapChainExtent = ChooseSwapChainExtent(m_swapChainInfo.m_capabilities);

    CreateSwapChain(m_surfaceFormat, m_presentMode, m_swapChainExtent);

    m_images = m_device.getSwapchainImagesKHR(m_swapChain);
    CreateImageViews(m_images, m_surfaceFormat.format);
    CreateFramebuffers();
}

void CVulkanRenderer::UpdateUniformBuffer(uint32_t currentImage, vk::Extent2D swapChainExtent) {
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    CUniformBufferObject ubo {};
    ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.proj = glm::perspective(glm::radians(40.0f), swapChainExtent.width / (float)swapChainExtent.height, 0.1f, 10.0f);
    ubo.proj[1][1] *= -1;
    memcpy(m_uniformBuffers[currentImage].mapped, &ubo, sizeof(ubo));
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
