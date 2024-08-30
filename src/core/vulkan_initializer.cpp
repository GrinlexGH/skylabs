#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include "vulkan_initializer.hpp"

#include "SDL.hpp"
#include "SDL_Vulkan.hpp"
#include "console.hpp"
#include "resourceloader.hpp"

#include <set>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

VKAPI_ATTR vk::Bool32 VKAPI_CALL vulkan_initializer::DebugCallback(
    vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    vk::DebugUtilsMessageTypeFlagBitsEXT messageType,
    const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData
) {
    UNUSED(messageType);
    UNUSED(pUserData);
    if (messageSeverity >= vk::DebugUtilsMessageSeverityFlagBitsEXT::eError) {
        Error << pCallbackData->pMessage << std::endl;
    } else if (messageSeverity >= vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning) {
        Warning << pCallbackData->pMessage << std::endl;
    } else {
        Msg << pCallbackData->pMessage << std::endl;
    }
    return VK_FALSE;
}

namespace vulkan_initializer {
#ifdef NDEBUG
    bool enableValidationLayers = false;
#else
    bool enableValidationLayers = true;
#endif

std::vector<std::string_view> FindMissingLayers(
    const std::vector<vk::LayerProperties>& availableLayers,
    const std::vector<const char*>& neededLayers
) {
    std::vector<std::string_view> missingLayers;
    for (const auto& neededLayer : neededLayers) {
        bool extFound = false;
        for (const auto& availableLayer : availableLayers) {
            if (strcmp(neededLayer, availableLayer.layerName) == 0) {
                extFound = true;
                break;
            }
        }
        if (!extFound) {
            missingLayers.push_back(neededLayer);
        }
    }
    return missingLayers;
}

std::vector<const char*> GetRequiredInstanceExtensions() {
    uint32_t extCount = 0;
    SDL::Vulkan::GetRequiredInstanceExtensions(&extCount, nullptr);
    std::vector<const char*> extensions(extCount);
    SDL::Vulkan::GetRequiredInstanceExtensions(&extCount, extensions.data());

    if (enableValidationLayers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
    return extensions;
}

std::vector<std::string_view> FindMissingExtensions(
    const std::vector<vk::ExtensionProperties>& availableExts,
    const std::vector<const char*>& neededExts
) {
    std::vector<std::string_view> missingExts;
    for (const auto& neededExt : neededExts) {
        bool extFound = false;
        for (const auto& availableExt : availableExts) {
            if (strcmp(neededExt, availableExt.extensionName) == 0) {
                extFound = true;
                break;
            }
        }
        if (!extFound) {
            missingExts.push_back(neededExt);
        }
    }
    return missingExts;
}

bool isDeviceSuitable(vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface) {
    CQueueFamilyIndices indices = FindQueueFamilies(physicalDevice, surface);
    bool extensionsSupported = FindMissingExtensions(physicalDevice.enumerateDeviceExtensionProperties(), g_deviceExtensions).empty();
    bool swapChainAdequate = false;
    if (extensionsSupported) {
        CSwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(physicalDevice, surface);
        swapChainAdequate = !swapChainSupport.m_formats.empty() &&
                            !swapChainSupport.m_presentModes.empty();
    }
    return extensionsSupported && swapChainAdequate && indices.isComplete();
}

CQueueFamilyIndices FindQueueFamilies(
    vk::PhysicalDevice physicalDevice,
    vk::SurfaceKHR surface
) {
    CQueueFamilyIndices indices {};
    std::vector<vk::QueueFamilyProperties> queueFamilies = physicalDevice.getQueueFamilyProperties();

    int i = 0;
    for (const auto& queueFamily : queueFamilies) {
        if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) {
            indices.m_graphicsFamily = i;
        }
        if (physicalDevice.getSurfaceSupportKHR(i, surface)) {
            indices.m_presentFamily = i;
        }
        if (indices.isComplete()) {
            break;
        }
        i++;
    }
    return indices;
}

CSwapChainSupportDetails QuerySwapChainSupport(
    vk::PhysicalDevice physicalDevice,
    vk::SurfaceKHR surface
) {
    CSwapChainSupportDetails details;
    details.m_capabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface);
    details.m_formats = physicalDevice.getSurfaceFormatsKHR(surface);
    details.m_presentModes = physicalDevice.getSurfacePresentModesKHR(surface);
    return details;
}

vk::ShaderModule CreateShaderModule(vk::Device device, const std::vector<char>& byteCode) {
    vk::ShaderModuleCreateInfo createInfo {};
    createInfo.codeSize = byteCode.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(byteCode.data());

    VkShaderModule shaderModule = device.createShaderModule(createInfo);
    return shaderModule;
}

vk::Instance CreateInstance(vk::DebugUtilsMessengerEXT& debugMessenger) {
    vk::Instance instance;
    VULKAN_HPP_DEFAULT_DISPATCHER.init();

    if (enableValidationLayers && !FindMissingLayers(vk::enumerateInstanceLayerProperties(), g_validationLayers).empty()) {
        enableValidationLayers = false;
        Warning << "Validation layers disabled because system doesn't have vulkan validation layers!\n";
    }

    vk::ApplicationInfo appInfo {
        "Skylabs", vk::makeApiVersion(0, 0, 0, 0),
        "Skylabs", vk::makeApiVersion(0, 0, 0, 0),
        vk::ApiVersion13
    };

    vk::InstanceCreateInfo createInfo {};
    createInfo.pApplicationInfo = &appInfo;
    auto requiredExtensions = GetRequiredInstanceExtensions();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
    createInfo.ppEnabledExtensionNames = requiredExtensions.data();

    if (auto notFoundedExt = FindMissingExtensions(vk::enumerateInstanceExtensionProperties(), requiredExtensions);
        !notFoundedExt.empty()) {
        std::string errorMsg = "System doesn't have necessary Vulkan extensions:\n";
        for (const auto& ext : notFoundedExt) {
            errorMsg += ext;
            errorMsg += '\n';
        }
        throw std::runtime_error(errorMsg);
    }

    vk::DebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo;
    if (enableValidationLayers) {
        debugMessengerCreateInfo.messageSeverity =
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eError |
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo;
        debugMessengerCreateInfo.messageType =
            vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
            vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
            vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
            vk::DebugUtilsMessageTypeFlagBitsEXT::eDeviceAddressBinding;
        debugMessengerCreateInfo.pfnUserCallback =
            reinterpret_cast<PFN_vkDebugUtilsMessengerCallbackEXT>(DebugCallback);
        createInfo.enabledLayerCount = static_cast<uint32_t>(g_validationLayers.size());
        createInfo.ppEnabledLayerNames = g_validationLayers.data();
        createInfo.pNext = &debugMessengerCreateInfo;
    } else {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
    }

    instance = vk::createInstance(createInfo);
    VULKAN_HPP_DEFAULT_DISPATCHER.init(instance);
    if (enableValidationLayers) {
        debugMessenger = instance.createDebugUtilsMessengerEXT(debugMessengerCreateInfo);
    }
    return instance;
}

vk::SurfaceFormatKHR ChooseSwapSurfaceFormat(
    const std::vector<vk::SurfaceFormatKHR>& availableFormats
) {
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == vk::Format::eB8G8R8A8Srgb &&
            availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

vk::PresentModeKHR ChooseSwapPresentMode(
    const std::vector<vk::PresentModeKHR>& availablePresentModes
) {
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == vk::PresentModeKHR::eFifo) { // todo: change
            return availablePresentMode;
        }
    }

    return vk::PresentModeKHR::eImmediate;
}

vk::Extent2D ChooseSwapExtent(
    IWindow* window,
    const vk::SurfaceCapabilitiesKHR& capabilities
) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    } else {
        int width, height;
        SDL::Vulkan::GetDrawableSize(static_cast<SDL_Window*>(window->GetHandle()), &width, &height);

        vk::Extent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        actualExtent.width = std::clamp(
            actualExtent.width,
            capabilities.minImageExtent.width,
            capabilities.maxImageExtent.width
        );
        actualExtent.height = std::clamp(
            actualExtent.height,
            capabilities.minImageExtent.height,
            capabilities.maxImageExtent.height
        );

        return actualExtent;
    }
}

vk::SurfaceKHR CreateSurface(vk::Instance instance, IWindow* window) {
    vk::SurfaceKHR surface;
    if (!SDL::Vulkan::CreateSurface(
            static_cast<SDL_Window*>(window->GetHandle()),
            instance,
            &surface
        )) {
        throw std::runtime_error("Failed to create window surface!\n");
    }
    return surface;
}

vk::PhysicalDevice PickPhysicalDevice(vk::Instance instance, vk::SurfaceKHR surface) {
    std::vector<vk::PhysicalDevice> devices = instance.enumeratePhysicalDevices();
    if (devices.size() == 0) {
        throw std::runtime_error("Failed to find GPUs with Vulkan support!\n");
    }

    vk::PhysicalDevice physicalDevice = VK_NULL_HANDLE;
    for (const auto& device : devices) {
        // todo: better implementation
        if (isDeviceSuitable(device, surface)) {
            physicalDevice = device;
            break;
        }
    }

    if (physicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("Failed to find a suitable GPU!\n");
    }

    return physicalDevice;
}

vk::Device CreateLogicalDevice(
    vk::PhysicalDevice physicalDevice,
    CQueueFamilyIndices queueIndices
) {
    if (enableValidationLayers && !FindMissingLayers(physicalDevice.enumerateDeviceLayerProperties(), g_validationLayers).empty()) {
        enableValidationLayers = false;
        Warning << "Validation layers disabled because system doesn't have vulkan validation layers!\n";
    }

    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {
        queueIndices.m_graphicsFamily.value(),
        queueIndices.m_presentFamily.value()
    };

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        vk::DeviceQueueCreateInfo queueCreateInfo {};
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    vk::PhysicalDeviceFeatures deviceFeatures {};

    vk::DeviceCreateInfo createInfo {};
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(g_deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = g_deviceExtensions.data();
    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(g_validationLayers.size());
        createInfo.ppEnabledLayerNames = g_validationLayers.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }

    vk::Device device = physicalDevice.createDevice(createInfo);
    return device;
}

vk::SwapchainKHR CreateSwapChain(
    vk::Device device,
    vk::SurfaceKHR surface,
    CQueueFamilyIndices queueIndices,
    const CSwapChainSupportDetails& swapChainSupport,
    vk::SurfaceFormatKHR surfaceFormat,
    vk::PresentModeKHR presentMode,
    vk::Extent2D extent
) {
    uint32_t imageCount = swapChainSupport.m_capabilities.minImageCount + 1;

    if (swapChainSupport.m_capabilities.maxImageCount > 0 && imageCount > swapChainSupport.m_capabilities.maxImageCount) {
        imageCount = swapChainSupport.m_capabilities.maxImageCount;
    }

    vk::SwapchainCreateInfoKHR createInfo {};
    createInfo.surface = surface;
    createInfo.pNext = nullptr;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;

    uint32_t queueFamilyIndices[] = {
        queueIndices.m_graphicsFamily.value(),
        queueIndices.m_presentFamily.value()
    };

    if (queueIndices.m_graphicsFamily != queueIndices.m_presentFamily) {
        createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = vk::SharingMode::eExclusive;
    }

    createInfo.preTransform = swapChainSupport.m_capabilities.currentTransform;
    createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    createInfo.presentMode = presentMode;
    createInfo.clipped = vk::True;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    vk::SwapchainKHR swapChain = device.createSwapchainKHR(createInfo);
    return swapChain;
}

std::vector<vk::ImageView> CreateImageViews(
    vk::Device device,
    const std::vector<vk::Image>& images,
    vk::Format imageFormat
) {
    std::vector<vk::ImageView> imageViews {};
    imageViews.resize(images.size());
    for (std::size_t i = 0; i < images.size(); i++) {
        vk::ImageViewCreateInfo createInfo {};
        createInfo.image = images[i];
        createInfo.viewType = vk::ImageViewType::e2D;
        createInfo.format = imageFormat;
        createInfo.components.r = vk::ComponentSwizzle::eIdentity;
        createInfo.components.g = vk::ComponentSwizzle::eIdentity;
        createInfo.components.b = vk::ComponentSwizzle::eIdentity;
        createInfo.components.a = vk::ComponentSwizzle::eIdentity;
        createInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;
        imageViews[i] = device.createImageView(createInfo);
    }
    return imageViews;
}

vk::RenderPass CreateRenderPass(vk::Device device, vk::Format imageFormat) {
    vk::AttachmentDescription colorAttachment {};
    colorAttachment.format = imageFormat;
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

    vk::RenderPassCreateInfo createInfo {};
    createInfo.attachmentCount = 1;
    createInfo.pAttachments = &colorAttachment;
    createInfo.subpassCount = 1;
    createInfo.pSubpasses = &subpassDesc;
    vk::SubpassDependency dependency {};
    dependency.srcSubpass = vk::SubpassExternal;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dependency.srcAccessMask = vk::AccessFlags {};
    dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
    createInfo.dependencyCount = 1;
    createInfo.pDependencies = &dependency;

    vk::RenderPass renderPass = device.createRenderPass(createInfo);
    return renderPass;
}

vk::PipelineLayout CreatePipelineLayout(vk::Device device) {
    vk::PipelineLayoutCreateInfo createInfo {};
    createInfo.setLayoutCount = 0;
    createInfo.pSetLayouts = nullptr;
    createInfo.pushConstantRangeCount = 0;
    createInfo.pPushConstantRanges = nullptr;

    vk::PipelineLayout pipelineLayout = device.createPipelineLayout(createInfo);

    return pipelineLayout;
}

vk::Pipeline CreatePipeline(vk::Device device, vk::PipelineLayout pipelineLayout, vk::RenderPass renderPass) {
    vk::GraphicsPipelineCreateInfo createInfo {};

    auto vertShaderCode = resource_loader::ReadFile("shaders/vert.spv");
    auto fragShaderCode = resource_loader::ReadFile("shaders/frag.spv");

    vk::ShaderModule vertexShader = CreateShaderModule(device, vertShaderCode);
    vk::ShaderModule fragmentShader = CreateShaderModule(device, fragShaderCode);

    vk::PipelineShaderStageCreateInfo vertShaderStageInfo {};
    vertShaderStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
    vertShaderStageInfo.module = vertexShader;
    vertShaderStageInfo.pName = "main";
    vk::PipelineShaderStageCreateInfo fragShaderStageInfo {};
    fragShaderStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
    fragShaderStageInfo.module = fragmentShader;
    fragShaderStageInfo.pName = "main";

    vk::PipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

    createInfo.stageCount = 2;
    createInfo.pStages = shaderStages;

    //==========
    vk::PipelineVertexInputStateCreateInfo vertexInputInfo {};
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.pVertexBindingDescriptions = nullptr;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;
    vertexInputInfo.pVertexAttributeDescriptions = nullptr;

    createInfo.pVertexInputState = &vertexInputInfo;

    //==========
    vk::PipelineInputAssemblyStateCreateInfo inputAssembly {};
    inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;
    inputAssembly.primitiveRestartEnable = vk::False;

    createInfo.pInputAssemblyState = &inputAssembly;

    //==========
    std::vector<vk::DynamicState> dynamicStates = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor
    };

    vk::PipelineDynamicStateCreateInfo dynamicState {};
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    createInfo.pDynamicState = &dynamicState;

    //==========
    vk::PipelineViewportStateCreateInfo viewportState {};
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    createInfo.pViewportState = &viewportState;

    //==========
    vk::PipelineRasterizationStateCreateInfo rasterizer {};
    rasterizer.depthClampEnable = vk::False;
    rasterizer.rasterizerDiscardEnable = vk::False;
    rasterizer.polygonMode = vk::PolygonMode::eFill;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = vk::CullModeFlagBits::eBack;
    rasterizer.frontFace = vk::FrontFace::eClockwise;
    rasterizer.depthBiasEnable = vk::False;
    rasterizer.depthBiasConstantFactor = 0.0f;
    rasterizer.depthBiasClamp = 0.0f;
    rasterizer.depthBiasSlopeFactor = 0.0f;

    createInfo.pRasterizationState = &rasterizer;

    //==========
    vk::PipelineMultisampleStateCreateInfo multisampling {};
    multisampling.sampleShadingEnable = vk::False;
    multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;
    multisampling.minSampleShading = 1.0f;
    multisampling.pSampleMask = nullptr;
    multisampling.alphaToCoverageEnable = vk::False;
    multisampling.alphaToOneEnable = vk::False;

    createInfo.pMultisampleState = &multisampling;

    //==========
    vk::PipelineColorBlendAttachmentState colorBlendAttachment {};
    colorBlendAttachment.colorWriteMask =
        vk::ColorComponentFlagBits::eR |
        vk::ColorComponentFlagBits::eG |
        vk::ColorComponentFlagBits::eB |
        vk::ColorComponentFlagBits::eA;
    colorBlendAttachment.blendEnable = vk::False;
    colorBlendAttachment.srcColorBlendFactor = vk::BlendFactor::eOne;
    colorBlendAttachment.dstColorBlendFactor = vk::BlendFactor::eZero;
    colorBlendAttachment.colorBlendOp = vk::BlendOp::eAdd;
    colorBlendAttachment.srcAlphaBlendFactor = vk::BlendFactor::eOne;
    colorBlendAttachment.dstAlphaBlendFactor = vk::BlendFactor::eZero;
    colorBlendAttachment.alphaBlendOp = vk::BlendOp::eAdd;

    vk::PipelineColorBlendStateCreateInfo colorBlending {};
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = vk::LogicOp::eCopy;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    createInfo.pDepthStencilState = nullptr;
    createInfo.pColorBlendState = &colorBlending;

    //==========
    createInfo.layout = pipelineLayout;
    createInfo.renderPass = renderPass;
    createInfo.subpass = 0;
    createInfo.basePipelineHandle = VK_NULL_HANDLE;
    createInfo.basePipelineIndex = -1;

    vk::Pipeline pipeline = device.createGraphicsPipeline(VK_NULL_HANDLE, createInfo).value;

    device.destroyShaderModule(fragmentShader);
    device.destroyShaderModule(vertexShader);

    return pipeline;
}

std::vector<vk::Framebuffer> CreateFramebuffers(
    vk::Device device,
    std::vector<vk::ImageView> imageViews,
    vk::RenderPass renderPass,
    vk::Extent2D extent
) {
    std::vector<vk::Framebuffer> frameBuffers {};
    frameBuffers.resize(imageViews.size());

    for (std::size_t i = 0; i < imageViews.size(); i++) {
        vk::ImageView attachments[] = {
            imageViews[i]
        };

        vk::FramebufferCreateInfo createInfo{};
        createInfo.renderPass = renderPass;
        createInfo.attachmentCount = 1;
        createInfo.pAttachments = attachments;
        createInfo.width = extent.width;
        createInfo.height = extent.height;
        createInfo.layers = 1;

        frameBuffers[i] = device.createFramebuffer(createInfo);
    }
    return frameBuffers;
}

vk::CommandPool CreateCommandPool(vk::Device device, CQueueFamilyIndices queueIndices) {
    vk::CommandPoolCreateInfo createInfo{};
    createInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
    createInfo.queueFamilyIndex = queueIndices.m_graphicsFamily.value();

    vk::CommandPool commandPool = device.createCommandPool(createInfo);
    return commandPool;
}

vk::CommandBuffer CreateCommandBuffer(vk::Device device, vk::CommandPool commandPool) {
    vk::CommandBufferAllocateInfo allocInfo {};
    allocInfo.commandPool = commandPool;
    allocInfo.level = vk::CommandBufferLevel::ePrimary;
    allocInfo.commandBufferCount = 1;

    vk::CommandBuffer commandBuffer = device.allocateCommandBuffers(allocInfo)[0];
    return commandBuffer;
}

} // vulkan_initializer
