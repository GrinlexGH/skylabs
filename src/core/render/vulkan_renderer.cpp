#include "console.hpp"
#include "vulkan_renderer.hpp"
#include "../SDL/SDL.hpp"
#include "../SDL/SDL_vulkan.hpp"
#include "unicode.hpp"
#include "resourceloader.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <tiny_obj_loader.h>

#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#include <vk_mem_alloc.hpp>

#include "../stb_image.h"
#include "../camera.hpp"

#include <fstream>
#include <filesystem>
#include <chrono>
#include <cassert>
#include <set>

constexpr std::size_t MAX_FRAMES_IN_FLIGHT = 2;

const std::string MODEL_PATH = "viking_room.obj";
const std::string TEXTURE_PATH = "viking_room.png";

//============
// CVulkanRenderer

bool CVulkanRenderer::Initialize(IWindow* window) {
    if (window == nullptr) {
        throw std::runtime_error("Cannot initialize vulkan renderer. Window is null!\n");
    }
    m_window = dynamic_cast<IVulkanWindow*>(window);

    try {
        VULKAN_HPP_DEFAULT_DISPATCHER.init();

        _InitializeInstanceExtensions();
        _InitializeInstance();

        _SetRequiredDeviceExtensions();
        m_window->CreateSurface(m_instance);
        _PickPhysicalDevice();
        m_queueFamiliesIndices = _FindQueueFamilies(m_physicalDevice);

        _InitializeDeviceExtensions();
        _initializeDevice();

        _CreateAllocator();

        m_graphicsQueue = m_device.getQueue(*m_queueFamiliesIndices.m_graphicsAndCompute, 0);
        m_presentQueue = m_device.getQueue(*m_queueFamiliesIndices.m_present, 0);

        m_surfaceCapabilities = m_physicalDevice.getSurfaceCapabilitiesKHR(m_window->GetSurface());
        m_currentSwapchainExtent = _ChooseSwapChainExtent();

        std::vector<vk::SurfaceFormatKHR> surfaceFormats = m_physicalDevice.getSurfaceFormatsKHR(m_window->GetSurface());
        m_currentSurfaceFormat = _ChooseSurfaceFormat(surfaceFormats);

        std::vector<vk::PresentModeKHR> presentModes = m_physicalDevice.getSurfacePresentModesKHR(m_window->GetSurface());
        m_currentPresentMode = _ChoosePresentMode(presentModes);

        _CreateSwapchain();
        m_images = m_device.getSwapchainImagesKHR(m_swapChain);
        _CreateImageViews(m_currentSurfaceFormat.format);

        _CreateRenderPass();

        _CreateDescriptorSetLayout();
        _CreatePipeline();

        _CreateColorResources();
        _CreateDepthResources();

        _CreateFramebuffers();

        _CreateCommandPool();
        _CreateCommandBuffers();

        LoadModel();

        _CreateVertexBuffer();
        _CreateIndexBuffer();

        _CreateUniformBuffers();

        _CreateTextureImage();
        _CreateTextureImageView(m_currentSurfaceFormat.format);
        _CreateTextureSampler();

        _CreateDescriptorPool();
        _CreateDescriptorSets();

        _CreateSyncObjects();
    } catch (const std::exception& e) {
        Error << e.what() << '\n';
        return false;
    }

    return true;
}

CVulkanRenderer::~CVulkanRenderer() {
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        m_device.destroySemaphore(m_imageAvailableSemaphores[i]);
        m_device.destroySemaphore(m_renderFinishedSemaphores[i]);
        m_device.destroyFence(m_inFlightFences[i]);
    }

    m_device.destroyDescriptorPool(m_descriptorPool);
    m_device.destroyDescriptorSetLayout(m_descriptorSetLayout);

    m_device.destroySampler(m_textureSampler);
    m_device.destroyImageView(m_textureImageView);
    m_allocator.destroyImage(m_textureImage.image, m_textureImage.allocation);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        m_allocator.unmapMemory(m_uniformBuffers[i].allocation);
        m_allocator.destroyBuffer(m_uniformBuffers[i].buffer, m_uniformBuffers[i].allocation);
    }

    m_allocator.destroyBuffer(m_indexBuffer.buffer, m_indexBuffer.allocation);
    m_allocator.destroyBuffer(m_vertexBuffer.buffer, m_vertexBuffer.allocation);

    m_device.destroyCommandPool(m_commandPool);

    _CleanupSwapchain();

    m_device.destroyPipeline(m_pipeline);
    m_device.destroyPipelineLayout(m_pipelineLayout);

    m_device.destroyRenderPass(m_renderPass);

    m_allocator.destroy();
    m_device.destroy();

    m_window->DestroySurface(m_instance);

    if (m_enableValidationLayer) {
        m_instance.destroyDebugUtilsMessengerEXT(m_debugMessenger);
    }

    m_instance.destroy();
}

//==========
// Instance
//==========

VKAPI_ATTR vk::Bool32 VKAPI_CALL CVulkanRenderer::DebugCallback(
    vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    vk::DebugUtilsMessageTypeFlagBitsEXT /*messageType*/,
    const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* /*pUserData*/
) {
    if (messageSeverity >= vk::DebugUtilsMessageSeverityFlagBitsEXT::eError) {
        Error << std::format("\n\nERROR: {}\n\n", pCallbackData->pMessage) << std::endl;
    } else if (messageSeverity >= vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning) {
        Warning << std::format("\nWarning: {}\n", pCallbackData->pMessage) << std::endl;
    } else {
        Msg << std::format("Info: {}", pCallbackData->pMessage) << std::endl;
    }
    return VK_FALSE;
}

void CVulkanRenderer::_InitializeInstanceExtensions() {
    // Setup all instance extensions
    for (const auto& ext : m_window->GetRequiredInstanceExtensions()) {
        m_instanceExtensions[ext] = true;
    }
    if (m_enableValidationLayer) {
        m_instanceExtensions[VK_EXT_DEBUG_UTILS_EXTENSION_NAME] = false;
    }

    // Find extensions that device supports
    std::vector<vk::ExtensionProperties> availableExtensions = vk::enumerateInstanceExtensionProperties();
    for (const auto& ext : m_instanceExtensions) {
        if (std::find_if(
            availableExtensions.begin(),
            availableExtensions.end(),
            [&](vk::ExtensionProperties availableExt){ return ext.first == availableExt.extensionName; }
        ) != availableExtensions.end()) {
            m_enabledInstanceExtensions.insert(ext.first.data());
        }
    }

    // Check for required extensions
    bool reqExtNotFound = false;
    for (const auto& ext : m_instanceExtensions) {
        if (!m_enabledInstanceExtensions.contains(ext.first)) {
            if (ext.second) {   // if required
                std::cerr << std::format("Required extension not found: {}\n", ext.first);
                reqExtNotFound = true;
            } else {
                std::cerr << std::format("Optional extension not found: {}\n", ext.first);
            }
        }
    }

    if (reqExtNotFound) {
        throw std::runtime_error("Cannot find required instance extensions!");
    }
}

static bool _IsValidationLayerFound() {
    std::vector<vk::LayerProperties> availableLayers = vk::enumerateInstanceLayerProperties();

    for (const auto& layer : availableLayers) {
        if (strcmp("VK_LAYER_KHRONOS_validation", layer.layerName) == 0) {
            return true;
        }
    }

    return false;
}

void CVulkanRenderer::_InitializeInstance() {
    std::vector<const char*> enabledExtensions {};
    for (const auto& ext : m_enabledInstanceExtensions) {
        enabledExtensions.push_back(ext.c_str());
    }

    if (m_enableValidationLayer &&
        (!_IsValidationLayerFound() || !m_enabledInstanceExtensions.contains(VK_EXT_DEBUG_UTILS_EXTENSION_NAME))
    ) {
        std::cerr << "Validation layer disabled.\n";
        m_enableValidationLayer = false;
    }

    std::vector<const char*> enabledLayers {};
    if (m_enableValidationLayer) {
        enabledLayers.push_back("VK_LAYER_KHRONOS_validation");
    }

    vk::InstanceCreateInfo instanceInfo {};

    vk::ApplicationInfo appInfo {};
    appInfo.pApplicationName = "Hotline Miami";
    appInfo.applicationVersion = vk::makeApiVersion(0, 0, 0, 0);
    appInfo.pEngineName = "Skylabs";
    appInfo.engineVersion = vk::makeApiVersion(0, 0, 0, 0);
    appInfo.apiVersion = vk::ApiVersion13;

    instanceInfo.pApplicationInfo = &appInfo;
    instanceInfo.enabledLayerCount = static_cast<uint32_t>(enabledLayers.size());
    instanceInfo.ppEnabledLayerNames = enabledLayers.data();
    instanceInfo.enabledExtensionCount = static_cast<uint32_t>(enabledExtensions.size());
    instanceInfo.ppEnabledExtensionNames = enabledExtensions.data();

    vk::DebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo {};
    if (m_enableValidationLayer) {
        debugMessengerCreateInfo.messageSeverity =
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;

        debugMessengerCreateInfo.messageType =
            vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
            vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation;

        debugMessengerCreateInfo.pfnUserCallback =
            reinterpret_cast<PFN_vkDebugUtilsMessengerCallbackEXT>(DebugCallback);

        instanceInfo.pNext = &debugMessengerCreateInfo;
    }

    m_instance = vk::createInstance(instanceInfo);
    VULKAN_HPP_DEFAULT_DISPATCHER.init(m_instance);

    m_debugMessenger = m_instance.createDebugUtilsMessengerEXT(debugMessengerCreateInfo);
}

//==========
// Physical device
//==========

void CVulkanRenderer::_SetRequiredDeviceExtensions() {
    m_deviceExtensions[VK_KHR_SWAPCHAIN_EXTENSION_NAME] = true;
    m_deviceExtensions[VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME] = true;
}

static std::string _GetDeviceVendorName(const CVulkanRenderer::DeviceVendor deviceVendor) {
    switch (deviceVendor) {
        case CVulkanRenderer::DeviceVendor::eAMD:
            return "AMD";
        case CVulkanRenderer::DeviceVendor::eImgTec:
            return "ImgTec";
        case CVulkanRenderer::DeviceVendor::eApple:
            return "Apple";
        case CVulkanRenderer::DeviceVendor::eNVIDIA:
            return "NVIDIA";
        case CVulkanRenderer::DeviceVendor::eARM:
            return "ARM";
        case CVulkanRenderer::DeviceVendor::eMicrosoft:
            return "Microsoft";
        case CVulkanRenderer::DeviceVendor::eQualcomm:
            return "Qualcomm";
        case CVulkanRenderer::DeviceVendor::eIntel:
            return "Intel";
        default:
            return "Unknown";
    }
}

static std::string _GetDeviceTypeName(vk::PhysicalDeviceType deviceType) {
    switch (deviceType) {
        case vk::PhysicalDeviceType::eDiscreteGpu:
            return "Discrete";
        case vk::PhysicalDeviceType::eIntegratedGpu:
            return "Integrated";
        case vk::PhysicalDeviceType::eVirtualGpu:
            return "Virtual";
        case vk::PhysicalDeviceType::eCpu:
            return "CPU";
        case vk::PhysicalDeviceType::eOther:
        default:
            return "Other";
    }
}

static bool _CheckExtensionSupport(vk::PhysicalDevice physicalDevice, const std::unordered_map<std::string, bool>& deviceExtensions) {
    std::unordered_set<std::string> enabledDeviceExtensions {};

    // Find extensions that device supports
    std::vector<vk::ExtensionProperties> availableExtensions = physicalDevice.enumerateDeviceExtensionProperties();
    for (const auto& ext : deviceExtensions) {
        if (std::find_if(
            availableExtensions.begin(),
            availableExtensions.end(),
            [&](vk::ExtensionProperties availableExt){ return ext.first == availableExt.extensionName; }
        ) != availableExtensions.end()) {
            enabledDeviceExtensions.insert(ext.first.data());
        }
    }

    // Check for required extensions
    bool reqExtNotFound = false;
    for (const auto& ext : deviceExtensions) {
        if (!enabledDeviceExtensions.contains(ext.first)) {
            if (ext.second) {   // if required
                std::cerr << std::format("Required extension not found: {}\n", ext.first);
                reqExtNotFound = true;
            } else {
                std::cerr << std::format("Optional extension not found: {}\n", ext.first);
            }
        }
    }

    if (reqExtNotFound) {
        return false;
    }

    return true;
}

CVulkanRenderer::CQueueFamilyIndices CVulkanRenderer::_FindQueueFamilies(vk::PhysicalDevice physicalDevice) {
    CQueueFamilyIndices indices {};
    std::vector<vk::QueueFamilyProperties> queueFamilies = physicalDevice.getQueueFamilyProperties();

    int i = 0;
    for (const auto& queueFamily : queueFamilies) {
        if ((queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) && (queueFamily.queueFlags & vk::QueueFlagBits::eCompute)) {
            indices.m_graphicsAndCompute = i;
        }

        if (physicalDevice.getSurfaceSupportKHR(i, m_window->GetSurface())) {
            indices.m_present = i;
        }

        if (indices.isComplete()) {
            break;
        }

        i++;
    }

    return indices;
}

bool CVulkanRenderer::_IsDeviceSuitable(vk::PhysicalDevice physicalDevice) {
    if (!_CheckExtensionSupport(physicalDevice, m_deviceExtensions)) {
        return false;
    }

    if (!_FindQueueFamilies(physicalDevice).isComplete()) {
        return false;
    }

    vk::PhysicalDeviceFeatures features = physicalDevice.getFeatures();
    if (!features.samplerAnisotropy &&
        !features.wideLines
    ) {
        return false;
    }

    return true;
}

static uint32_t _GetDeviceTypeScore(vk::PhysicalDeviceType deviceType) {
    switch (deviceType) {
        case vk::PhysicalDeviceType::eDiscreteGpu:
            return 5;
        case vk::PhysicalDeviceType::eIntegratedGpu:
            return 4;
        case vk::PhysicalDeviceType::eVirtualGpu:
            return 3;
        case vk::PhysicalDeviceType::eCpu:
            return 2;
        case vk::PhysicalDeviceType::eOther:
        default:
            return 1;
    }
}

static vk::SampleCountFlagBits _GetMaxUsableSampleCount(vk::PhysicalDevice physicalDevice) {
    vk::PhysicalDeviceProperties physicalDeviceProperties = physicalDevice.getProperties();

    vk::SampleCountFlags counts =
        physicalDeviceProperties.limits.framebufferColorSampleCounts &
        physicalDeviceProperties.limits.framebufferDepthSampleCounts;

    if (counts & vk::SampleCountFlagBits::e64) {
        return vk::SampleCountFlagBits::e64;
    }
    if (counts & vk::SampleCountFlagBits::e32) {
        return vk::SampleCountFlagBits::e32;
    }
    if (counts & vk::SampleCountFlagBits::e16) {
        return vk::SampleCountFlagBits::e16;
    }
    if (counts & vk::SampleCountFlagBits::e8) {
        return vk::SampleCountFlagBits::e8;
    }
    if (counts & vk::SampleCountFlagBits::e4) {
        return vk::SampleCountFlagBits::e4;
    }
    if (counts & vk::SampleCountFlagBits::e2) {
        return vk::SampleCountFlagBits::e2;
    }

    return vk::SampleCountFlagBits::e1;
}

void CVulkanRenderer::_PickPhysicalDevice() {
    std::vector<vk::PhysicalDevice> physicalDevices = m_instance.enumeratePhysicalDevices();

    std::cout << "Devices:\n";

    uint32_t deviceTypeScore = 0;
    for (std::size_t i = 0; i < physicalDevices.size(); i++) {
        vk::PhysicalDeviceProperties properties = physicalDevices[i].getProperties();

        std::string name = properties.deviceName;
        std::string vendor = _GetDeviceVendorName(DeviceVendor(properties.vendorID));
        std::string type = _GetDeviceTypeName(properties.deviceType);
        std::cout << std::format("#{}: {} {} - {}:\n", i, vendor, name, type);

        if (_IsDeviceSuitable(physicalDevices[i])) {
            uint32_t optionScore = _GetDeviceTypeScore(properties.deviceType);
            if (optionScore > deviceTypeScore) {
                m_physicalDevice = physicalDevices[i];
                m_msaaSamples = _GetMaxUsableSampleCount(m_physicalDevice);
                deviceTypeScore = optionScore;
            }
        }
    }

    if (m_physicalDevice == nullptr) {
        throw std::runtime_error("No suitable gpu found!");
    }

    std::cout << "Device picked!\n";
}

//==========
// Logical device
//==========

void CVulkanRenderer::_InitializeDeviceExtensions() {
    // Find extensions that device supports
    std::vector<vk::ExtensionProperties> availableExtensions = m_physicalDevice.enumerateDeviceExtensionProperties();
    for (const auto& ext : m_deviceExtensions) {
        if (std::find_if(
            availableExtensions.begin(),
            availableExtensions.end(),
            [&](vk::ExtensionProperties availableExt){ return ext.first == availableExt.extensionName; }
        ) != availableExtensions.end()) {
            m_enabledDeviceExtensions.insert(ext.first.data());
        }
    }
}

void CVulkanRenderer::_initializeDevice() {
    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos {};
    std::set<uint32_t> uniqueQueueFamilies = {
        m_queueFamiliesIndices.m_graphicsAndCompute.value(),
        m_queueFamiliesIndices.m_present.value()
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

    vk::PhysicalDeviceFeatures requestedDeviceFeatures {};
    requestedDeviceFeatures.samplerAnisotropy = true;

    vk::DeviceCreateInfo deviceInfo {};
    deviceInfo.pQueueCreateInfos = queueCreateInfos.data();
    deviceInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    deviceInfo.pEnabledFeatures = &requestedDeviceFeatures;
    deviceInfo.enabledExtensionCount = static_cast<uint32_t>(enabledExtensions.size());
    deviceInfo.ppEnabledExtensionNames = enabledExtensions.data();

    m_device = m_physicalDevice.createDevice(deviceInfo);
}

//==========
// Other
//==========

void CVulkanRenderer::_CreateAllocator() {
    vma::AllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.flags = vma::AllocatorCreateFlagBits::eExtMemoryBudget;
    allocatorInfo.vulkanApiVersion = vk::ApiVersion13;
    allocatorInfo.physicalDevice = m_physicalDevice;
    allocatorInfo.device = m_device;
    allocatorInfo.instance = m_instance;

    vma::VulkanFunctions vulkanFunctions = vma::functionsFromDispatcher();
    allocatorInfo.pVulkanFunctions = &vulkanFunctions;

    m_allocator = vma::createAllocator(allocatorInfo);
}

//==========
// Swapchain
//==========

vk::SurfaceFormatKHR CVulkanRenderer::_ChooseSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& formats) {
    for (const auto& availableFormat : formats) {
        if (availableFormat.format == vk::Format::eR8G8B8A8Srgb &&
            availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear
        ) {
            return availableFormat;
        }
    }

    std::cerr << "Surface format was not selected!\n";
    return formats[0];
}

vk::PresentModeKHR CVulkanRenderer::_ChoosePresentMode(const std::vector<vk::PresentModeKHR>& presentModes) {
    for (const auto& availablePresentMode : presentModes) { // VSYNC
        if (availablePresentMode == vk::PresentModeKHR::eImmediate) {
            return availablePresentMode;
        }
    }

    std::cerr << "Correct present mode was not selected!\n";
    return presentModes[0];
}

vk::Extent2D CVulkanRenderer::_ChooseSwapChainExtent() {
    if (m_surfaceCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return m_surfaceCapabilities.currentExtent;
    } else {
        int width = 0, height = 0;
        m_window->GetDrawableSize(&width, &height);

        vk::Extent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        actualExtent.width = std::clamp(
            actualExtent.width,
            m_surfaceCapabilities.minImageExtent.width,
            m_surfaceCapabilities.maxImageExtent.width
        );
        actualExtent.height = std::clamp(
            actualExtent.height,
            m_surfaceCapabilities.minImageExtent.height,
            m_surfaceCapabilities.maxImageExtent.height
        );

        return actualExtent;
    }
}

void CVulkanRenderer::_CreateSwapchain() {
    uint32_t imageCount = m_surfaceCapabilities.minImageCount + 1;

    if (m_surfaceCapabilities.maxImageCount > 0 && imageCount > m_surfaceCapabilities.maxImageCount) {
        imageCount = m_surfaceCapabilities.maxImageCount;
    }

    vk::SwapchainCreateInfoKHR swapChainInfo {};
    swapChainInfo.surface = m_window->GetSurface();
    swapChainInfo.minImageCount = imageCount;
    swapChainInfo.imageFormat = m_currentSurfaceFormat.format;
    swapChainInfo.imageColorSpace = m_currentSurfaceFormat.colorSpace;
    swapChainInfo.imageExtent = m_currentSwapchainExtent;
    swapChainInfo.imageArrayLayers = 1;
    swapChainInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;

    uint32_t queueFamilyIndices[] = {
        m_queueFamiliesIndices.m_graphicsAndCompute.value(),
        m_queueFamiliesIndices.m_present.value()
    };

    if (m_queueFamiliesIndices.m_graphicsAndCompute != m_queueFamiliesIndices.m_present) {
        swapChainInfo.imageSharingMode = vk::SharingMode::eConcurrent;
        swapChainInfo.queueFamilyIndexCount = 2;
        swapChainInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        swapChainInfo.imageSharingMode = vk::SharingMode::eExclusive;
    }

    swapChainInfo.preTransform = m_surfaceCapabilities.currentTransform;
    swapChainInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    swapChainInfo.presentMode = m_currentPresentMode;
    swapChainInfo.clipped = vk::False;

    m_swapChain = m_device.createSwapchainKHR(swapChainInfo);
}

void CVulkanRenderer::_CleanupSwapchain() {
    m_device.destroyImageView(m_colorImageView);
    m_allocator.destroyImage(m_colorImage.image, m_colorImage.allocation);

    m_device.destroyImageView(m_depthImageView);
    m_allocator.destroyImage(m_depthImage.image, m_depthImage.allocation);

    for (size_t i = 0; i < m_frameBuffers.size(); ++i) {
        m_device.destroyFramebuffer(m_frameBuffers[i]);
    }

    for (size_t i = 0; i < m_imageViews.size(); ++i) {
        m_device.destroyImageView(m_imageViews[i]);
    }

    m_device.destroySwapchainKHR(m_swapChain);
}

void CVulkanRenderer::_RecreateSwapchain() {
    m_device.waitIdle();

    _CleanupSwapchain();
// todo: implicitly define dependencies
    m_surfaceCapabilities = m_physicalDevice.getSurfaceCapabilitiesKHR(m_window->GetSurface());
    m_currentSwapchainExtent = _ChooseSwapChainExtent();

    std::vector<vk::SurfaceFormatKHR> surfaceFormats = m_physicalDevice.getSurfaceFormatsKHR(m_window->GetSurface());
    m_currentSurfaceFormat = _ChooseSurfaceFormat(surfaceFormats);

    std::vector<vk::PresentModeKHR> presentModes = m_physicalDevice.getSurfacePresentModesKHR(m_window->GetSurface());
    m_currentPresentMode = _ChoosePresentMode(presentModes);

    _CreateSwapchain();

    m_images = m_device.getSwapchainImagesKHR(m_swapChain);
    _CreateImageViews(m_currentSurfaceFormat.format);
    _CreateColorResources();
    _CreateDepthResources();
    _CreateFramebuffers();
}

vk::ImageView CVulkanRenderer::_CreateImageView(vk::Image image, vk::Format format, vk::ImageAspectFlags aspectFlags, uint32_t mipLevels) {
    vk::ImageViewCreateInfo imageViewInfo {};
    imageViewInfo.image = image;
    imageViewInfo.viewType = vk::ImageViewType::e2D;
    imageViewInfo.format = format;
    imageViewInfo.subresourceRange.aspectMask = aspectFlags;
    imageViewInfo.subresourceRange.baseMipLevel = 0;
    imageViewInfo.subresourceRange.levelCount = mipLevels;
    imageViewInfo.subresourceRange.baseArrayLayer = 0;
    imageViewInfo.subresourceRange.layerCount = 1;

    return m_device.createImageView(imageViewInfo);
}

void CVulkanRenderer::_CreateImageViews(vk::Format format) {
    m_imageViews.resize(m_images.size());

    for (std::size_t i = 0; i < m_images.size(); ++i) {
        m_imageViews[i] = _CreateImageView(m_images[i], format, vk::ImageAspectFlagBits::eColor, 1);
    }
}

void CVulkanRenderer::_CreateRenderPass() {
    vk::AttachmentDescription colorAttachment {};
    colorAttachment.format = m_currentSurfaceFormat.format;
    colorAttachment.samples = m_msaaSamples;
    colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
    colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
    colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    colorAttachment.initialLayout = vk::ImageLayout::eUndefined;
    colorAttachment.finalLayout = vk::ImageLayout::eColorAttachmentOptimal;

    vk::AttachmentDescription depthAttachment {};
    depthAttachment.format = _GetDepthFormat();
    depthAttachment.samples = m_msaaSamples;
    depthAttachment.loadOp = vk::AttachmentLoadOp::eClear;
    depthAttachment.storeOp = vk::AttachmentStoreOp::eDontCare;
    depthAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    depthAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    depthAttachment.initialLayout = vk::ImageLayout::eUndefined;
    depthAttachment.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

    vk::AttachmentDescription colorAttachmentResolve {};
    colorAttachmentResolve.format = m_currentSurfaceFormat.format;
    colorAttachmentResolve.samples = vk::SampleCountFlagBits::e1;
    colorAttachmentResolve.loadOp = vk::AttachmentLoadOp::eDontCare;
    colorAttachmentResolve.storeOp = vk::AttachmentStoreOp::eStore;
    colorAttachmentResolve.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    colorAttachmentResolve.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    colorAttachmentResolve.initialLayout = vk::ImageLayout::eUndefined;
    colorAttachmentResolve.finalLayout = vk::ImageLayout::ePresentSrcKHR;

    vk::AttachmentReference colorAttachmentRef {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

    vk::AttachmentReference depthAttachmentRef {};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

    vk::AttachmentReference colorAttachmentResolveRef {};
    colorAttachmentResolveRef.attachment = 2;
    colorAttachmentResolveRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

    vk::SubpassDescription subpassDesc {};
    subpassDesc.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
    subpassDesc.colorAttachmentCount = 1;
    subpassDesc.pColorAttachments = &colorAttachmentRef;
    subpassDesc.pDepthStencilAttachment = &depthAttachmentRef;
    subpassDesc.pResolveAttachments = &colorAttachmentResolveRef;

    vk::SubpassDependency dependency {};
    dependency.srcSubpass = vk::SubpassExternal;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests;
    dependency.srcAccessMask = vk::AccessFlagBits::eNone;
    dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests;
    dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite;

    std::array<vk::AttachmentDescription, 3> attachments = { colorAttachment, depthAttachment, colorAttachmentResolve };
    vk::RenderPassCreateInfo renderPassInfo {};
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpassDesc;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    m_renderPass = m_device.createRenderPass(renderPassInfo);
}

void CVulkanRenderer::_CreateDescriptorSetLayout() {
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

    m_descriptorSetLayout = m_device.createDescriptorSetLayout(layoutInfo);
}

vk::ShaderModule CVulkanRenderer::_CreateShaderModule(const std::vector<char>& byteCode) {
    vk::ShaderModuleCreateInfo shaderModuleInfo {};
    shaderModuleInfo.codeSize = byteCode.size();
    shaderModuleInfo.pCode = reinterpret_cast<const uint32_t*>(byteCode.data());

    vk::ShaderModule shaderModule = m_device.createShaderModule(shaderModuleInfo);
    return shaderModule;
}

void CVulkanRenderer::_CreatePipeline() {
    vk::GraphicsPipelineCreateInfo pipelineInfo {};

    auto vertShaderCode = resource_loader::ReadFile("shaders/vert.spv");
    auto fragShaderCode = resource_loader::ReadFile("shaders/frag.spv");
    auto computeShaderCode = resource_loader::ReadFile("shaders/compute.spv");

    vk::ShaderModule vertexShader = _CreateShaderModule(vertShaderCode);
    vk::ShaderModule fragmentShader = _CreateShaderModule(fragShaderCode);
    vk::ShaderModule computeShaderModule = _CreateShaderModule(computeShaderCode);

    vk::PipelineShaderStageCreateInfo vertShaderStageInfo {};
    vertShaderStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
    vertShaderStageInfo.module = vertexShader;
    vertShaderStageInfo.pName = "main";
    vk::PipelineShaderStageCreateInfo fragShaderStageInfo {};
    fragShaderStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
    fragShaderStageInfo.module = fragmentShader;
    fragShaderStageInfo.pName = "main";
    vk::PipelineShaderStageCreateInfo computeShaderStageInfo {};
    computeShaderStageInfo.stage = vk::ShaderStageFlagBits::eCompute;
    computeShaderStageInfo.module = computeShaderModule;
    computeShaderStageInfo.pName = "main";

    vk::PipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;

    //==========
    vk::VertexInputBindingDescription bindingDescription = CVertex::getBindingDescription();
    std::array<vk::VertexInputAttributeDescription, 3> attributeDescriptions = CVertex::getAttributeDescriptions();

    vk::PipelineVertexInputStateCreateInfo vertexInputInfo {};
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
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
    vk::PipelineDepthStencilStateCreateInfo depthStencil {};
    depthStencil.depthTestEnable = vk::True;
    depthStencil.depthWriteEnable = vk::True;
    depthStencil.depthCompareOp = vk::CompareOp::eLess;
    depthStencil.depthBoundsTestEnable = vk::False;
    depthStencil.minDepthBounds = 0.0f;
    depthStencil.maxDepthBounds = 1.0f;
    depthStencil.stencilTestEnable = vk::False;
    depthStencil.front = vk::StencilOpState {};
    depthStencil.back = vk::StencilOpState {};

    pipelineInfo.pDepthStencilState = &depthStencil;

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
    multisampling.rasterizationSamples = m_msaaSamples;

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

void CVulkanRenderer::_CreateColorResources() {
    vk::Format colorFormat = m_currentSurfaceFormat.format;

    m_colorImage = _CreateImage(
        m_currentSwapchainExtent.width,
        m_currentSwapchainExtent.height,
        1, m_msaaSamples, colorFormat,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eTransientAttachment | vk::ImageUsageFlagBits::eColorAttachment,
        vk::MemoryPropertyFlagBits::eDeviceLocal
    );

    m_colorImageView = _CreateImageView(m_colorImage.image, colorFormat, vk::ImageAspectFlagBits::eColor, 1);
}

vk::Format CVulkanRenderer::_GetSupportedFormat(
    const std::vector<vk::Format>& candidates,
    vk::ImageTiling tiling,
    vk::FormatFeatureFlags features
) {
    for (vk::Format format : candidates) {
        vk::FormatProperties props = m_physicalDevice.getFormatProperties(format);
        if (tiling == vk::ImageTiling::eLinear && (props.linearTilingFeatures & features) == features) {
            return format;
        } else if (tiling == vk::ImageTiling::eOptimal && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    throw std::runtime_error("Failed to find supported format!");
}

vk::Format CVulkanRenderer::_GetDepthFormat() {
    return _GetSupportedFormat(
        { vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint },
        vk::ImageTiling::eOptimal,
        vk::FormatFeatureFlagBits::eDepthStencilAttachment
    );
}

void CVulkanRenderer::_CreateDepthResources() {
    vk::Format depthFormat = _GetDepthFormat();
    m_depthImage = _CreateImage(
        m_currentSwapchainExtent.width,
        m_currentSwapchainExtent.height,
        1,
        m_msaaSamples,
        depthFormat,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eDepthStencilAttachment,
        vk::MemoryPropertyFlagBits::eDeviceLocal
    );
    m_depthImageView = _CreateImageView(m_depthImage.image, depthFormat, vk::ImageAspectFlagBits::eDepth, 1);
}

void CVulkanRenderer::_CreateFramebuffers() {
    m_frameBuffers.resize(m_imageViews.size());

    for (std::size_t i = 0; i < m_imageViews.size(); i++) {
        std::array<vk::ImageView, 3> attachments = {
            m_colorImageView,
            m_depthImageView,
            m_imageViews[i],
        };
        vk::FramebufferCreateInfo frameBufferInfo {};
        frameBufferInfo.renderPass = m_renderPass;
        frameBufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        frameBufferInfo.pAttachments = attachments.data();
        frameBufferInfo.width = m_currentSwapchainExtent.width;
        frameBufferInfo.height = m_currentSwapchainExtent.height;
        frameBufferInfo.layers = 1;

        m_frameBuffers[i] = m_device.createFramebuffer(frameBufferInfo);
    }
}

void CVulkanRenderer::_CreateCommandPool() {
    vk::CommandPoolCreateInfo commandPoolInfo {};
    commandPoolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
    commandPoolInfo.queueFamilyIndex = m_queueFamiliesIndices.m_graphicsAndCompute.value();

    m_commandPool = m_device.createCommandPool(commandPoolInfo);
}

void CVulkanRenderer::_CreateCommandBuffers() {
    m_commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    vk::CommandBufferAllocateInfo cmdBufferAllocInfo {};
    cmdBufferAllocInfo.commandPool = m_commandPool;
    cmdBufferAllocInfo.level = vk::CommandBufferLevel::ePrimary;
    cmdBufferAllocInfo.commandBufferCount = static_cast<uint32_t>(m_commandBuffers.size());

    m_commandBuffers = m_device.allocateCommandBuffers(cmdBufferAllocInfo);
}

CVulkanRenderer::CBuffer CVulkanRenderer::_CreateBuffer(
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

vk::CommandBuffer CVulkanRenderer::_BeginSingleTimeCommands() {
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

void CVulkanRenderer::_EndSingleTimeCommands(vk::CommandBuffer commandBuffer) {
    commandBuffer.end();

    vk::SubmitInfo submitInfo {};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    m_graphicsQueue.submit(submitInfo);
    m_graphicsQueue.waitIdle();

    m_device.freeCommandBuffers(m_commandPool, commandBuffer);
}

void CVulkanRenderer::_CopyBuffer(
    vk::Buffer srcBuffer,
    vk::Buffer dstBuffer,
    vk::DeviceSize size
) {
    vk::CommandBuffer commandBuffer = _BeginSingleTimeCommands();

    vk::BufferCopy copyRegion {};
    copyRegion.size = size;
    commandBuffer.copyBuffer(srcBuffer, dstBuffer, copyRegion);

    _EndSingleTimeCommands(commandBuffer);
}

void CVulkanRenderer::LoadModel() {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, MODEL_PATH.c_str())) {
        throw std::runtime_error(warn + err);
    }

    std::unordered_map<CVertex, uint32_t> uniqueVertices {};

    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            CVertex vertex {};

            vertex.pos = {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]
            };

            vertex.texCoord = {
                attrib.texcoords[2 * index.texcoord_index + 0],
                1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
            };

            vertex.color = {1.0f, 1.0f, 1.0f};

            if (uniqueVertices.count(vertex) == 0) {
                uniqueVertices[vertex] = static_cast<uint32_t>(m_vertices.size());
                m_vertices.push_back(vertex);
            }

            m_indices.push_back(uniqueVertices[vertex]);
        }
    }
}

void CVulkanRenderer::_CreateVertexBuffer() {
    vk::DeviceSize bufferSize = sizeof(m_vertices[0]) * m_vertices.size();

    CBuffer stagingBuffer = _CreateBuffer(
        bufferSize,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
        vma::AllocationCreateFlagBits::eHostAccessRandom
    );

    void* data = m_allocator.mapMemory(stagingBuffer.allocation);
        memcpy(data, m_vertices.data(), (std::size_t)bufferSize);
    m_allocator.unmapMemory(stagingBuffer.allocation);

    m_vertexBuffer = _CreateBuffer(
        bufferSize,
        vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        {}
    );

    _CopyBuffer(stagingBuffer.buffer, m_vertexBuffer.buffer, bufferSize);

    m_allocator.destroyBuffer(stagingBuffer.buffer, stagingBuffer.allocation);
}

void CVulkanRenderer::_CreateIndexBuffer() {
    vk::DeviceSize bufferSize = sizeof(m_indices[0]) * m_indices.size();

    CBuffer stagingBuffer = _CreateBuffer(
        bufferSize,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
        vma::AllocationCreateFlagBits::eHostAccessRandom
    );

    void* data = m_allocator.mapMemory(stagingBuffer.allocation);
        memcpy(data, m_indices.data(), (std::size_t)bufferSize);
    m_allocator.unmapMemory(stagingBuffer.allocation);

    m_indexBuffer = _CreateBuffer(
        bufferSize,
        vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        {}
    );

    _CopyBuffer(stagingBuffer.buffer, m_indexBuffer.buffer, bufferSize);

    m_allocator.destroyBuffer(stagingBuffer.buffer, stagingBuffer.allocation);
}

void CVulkanRenderer::_CreateUniformBuffers() {
    vk::DeviceSize bufferSize = sizeof(CUniformBufferObject);

    m_uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    m_uniformBuffersData.resize(MAX_FRAMES_IN_FLIGHT);
    for (std::size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        m_uniformBuffers[i] = _CreateBuffer(
            bufferSize,
            vk::BufferUsageFlagBits::eUniformBuffer,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
            vma::AllocationCreateFlagBits::eHostAccessRandom
        );
        m_uniformBuffersData[i] = m_allocator.mapMemory(m_uniformBuffers[i].allocation);
    }
}

void CVulkanRenderer::_CreateDescriptorPool() {
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

void CVulkanRenderer::_CreateDescriptorSets() {
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

CVulkanRenderer::CImage CVulkanRenderer::_CreateImage(
    uint32_t width,
    uint32_t height,
    uint32_t mipLevels,
    vk::SampleCountFlagBits numSamples,
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
    imageInfo.mipLevels = mipLevels;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = vk::ImageLayout::eUndefined;
    imageInfo.usage = usage;
    imageInfo.sharingMode = vk::SharingMode::eExclusive;
    imageInfo.samples = numSamples;

    vma::AllocationCreateInfo allocInfo {};
    allocInfo.usage = vma::MemoryUsage::eAuto;
    allocInfo.requiredFlags = properties;

    std::pair<vk::Image, vma::Allocation> image = m_allocator.createImage(imageInfo, allocInfo);
    result.image = image.first;
    result.allocation = image.second;

    return result;
}

void CVulkanRenderer::_TransitionImageLayout(
    vk::Image image,
    vk::Format format,
    vk::ImageLayout oldLayout,
    vk::ImageLayout newLayout,
    uint32_t mipLevels
) {
    (void)format;
    vk::CommandBuffer commandBuffer = _BeginSingleTimeCommands();

    vk::ImageMemoryBarrier barrier {};
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
    barrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = mipLevels;
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

    _EndSingleTimeCommands(commandBuffer);
}

void CVulkanRenderer::_CopyBufferToImage(
    vk::Buffer buffer,
    vk::Image image,
    uint32_t width,
    uint32_t height
) {
    vk::CommandBuffer commandBuffer = _BeginSingleTimeCommands();

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

    _EndSingleTimeCommands(commandBuffer);
}

void CVulkanRenderer::_GenerateMipMaps(vk::Image image, vk::Format format, int32_t texWidth, int32_t texHeight, uint32_t mipLevels) {
    vk::FormatProperties formatProperties = m_physicalDevice.getFormatProperties(format);

    if (!(formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear)) {
        throw std::runtime_error("texture image format does not support linear blitting!");
    }

    vk::CommandBuffer commandBuffer = _BeginSingleTimeCommands();

    vk::ImageMemoryBarrier barrier {};
    barrier.image = image;
    barrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
    barrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;
    barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;

    int32_t mipWidth = texWidth;
    int32_t mipHeight = texHeight;

    for (uint32_t i = 1; i < mipLevels; i++) {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
        barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;

        commandBuffer.pipelineBarrier(
            vk::PipelineStageFlagBits::eTransfer,
            vk::PipelineStageFlagBits::eTransfer,
            vk::DependencyFlags {},
            0, nullptr,
            0, nullptr,
            1, &barrier
        );

        vk::ImageBlit blit {};
        blit.srcOffsets[0] = vk::Offset3D { 0, 0, 0 };
        blit.srcOffsets[1] = vk::Offset3D { mipWidth, mipHeight, 1 };
        blit.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;
        blit.dstOffsets[0] = vk::Offset3D { 0, 0, 0 };
        blit.dstOffsets[1] = vk::Offset3D { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
        blit.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1;

        commandBuffer.blitImage(
            image, vk::ImageLayout::eTransferSrcOptimal,
            image, vk::ImageLayout::eTransferDstOptimal,
            1, &blit,
            vk::Filter::eLinear
        );

        barrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
        barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

        commandBuffer.pipelineBarrier(
            vk::PipelineStageFlagBits::eTransfer,
            vk::PipelineStageFlagBits::eFragmentShader,
            vk::DependencyFlags {}, 0, nullptr,
            0, nullptr, 1,
            &barrier
        );

        if (mipWidth > 1)
            mipWidth /= 2;
        if (mipHeight > 1)
            mipHeight /= 2;
    }

    barrier.subresourceRange.baseMipLevel = mipLevels - 1;
    barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
    barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
    barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

    commandBuffer.pipelineBarrier(
        vk::PipelineStageFlagBits::eTransfer,
        vk::PipelineStageFlagBits::eFragmentShader,
        vk::DependencyFlags {}, 0, nullptr,
        0, nullptr, 1,
        &barrier
    );

    _EndSingleTimeCommands(commandBuffer);
}

void CVulkanRenderer::_CreateTextureImage() {
    int texWidth = 0, texHeight = 0, texChannels = 0;
    stbi_uc* pixels = stbi_load(TEXTURE_PATH.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    if (!pixels) {
        throw std::runtime_error("failed to load texture image!");
    }

    m_mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

    vk::DeviceSize imageSize = texWidth * texHeight * 4;

    CBuffer stagingBuffer = _CreateBuffer(
        imageSize,
        vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
        vma::AllocationCreateFlagBits::eHostAccessRandom
    );

    void* data = m_allocator.mapMemory(stagingBuffer.allocation);
        memcpy(data, pixels, static_cast<size_t>(imageSize));
    m_allocator.unmapMemory(stagingBuffer.allocation);

    stbi_image_free(pixels);

    m_textureImage = _CreateImage(
        texWidth,
        texHeight,
        m_mipLevels,
        vk::SampleCountFlagBits::e1,
        vk::Format::eR8G8B8A8Srgb,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
        vk::MemoryPropertyFlagBits::eDeviceLocal
    );

    _TransitionImageLayout(
        m_textureImage.image,
        vk::Format::eR8G8B8A8Srgb,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eTransferDstOptimal,
        m_mipLevels
    );

    _CopyBufferToImage(
        stagingBuffer.buffer,
        m_textureImage.image,
        static_cast<uint32_t>(texWidth),
        static_cast<uint32_t>(texHeight)
    );

    _GenerateMipMaps(m_textureImage.image, vk::Format::eR8G8B8A8Srgb, texWidth, texHeight, m_mipLevels);

    m_allocator.destroyBuffer(stagingBuffer.buffer, stagingBuffer.allocation);
}

void CVulkanRenderer::_CreateTextureImageView(vk::Format format) {
    m_textureImageView = _CreateImageView(m_textureImage.image, format, vk::ImageAspectFlagBits::eColor, m_mipLevels);
}

void CVulkanRenderer::_CreateTextureSampler() {
    vk::SamplerCreateInfo samplerInfo {};
    samplerInfo.magFilter = vk::Filter::eNearest;
    samplerInfo.minFilter = vk::Filter::eNearest;
    samplerInfo.addressModeU = vk::SamplerAddressMode::eRepeat;
    samplerInfo.addressModeV = vk::SamplerAddressMode::eRepeat;
    samplerInfo.addressModeW = vk::SamplerAddressMode::eRepeat;
    samplerInfo.anisotropyEnable = vk::True;
    samplerInfo.maxAnisotropy = m_physicalDevice.getProperties().limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = vk::BorderColor::eIntOpaqueBlack;
    samplerInfo.unnormalizedCoordinates = vk::False;
    samplerInfo.compareEnable = vk::False;
    samplerInfo.compareOp = vk::CompareOp::eAlways;
    samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = static_cast<float>(m_mipLevels);

    m_textureSampler = m_device.createSampler(samplerInfo);
}

void CVulkanRenderer::_CreateSyncObjects() {
    m_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    for (std::size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        m_imageAvailableSemaphores[i] = m_device.createSemaphore(vk::SemaphoreCreateInfo {});
        m_renderFinishedSemaphores[i] = m_device.createSemaphore(vk::SemaphoreCreateInfo {});
        m_inFlightFences[i] = m_device.createFence(vk::FenceCreateInfo { vk::FenceCreateFlagBits::eSignaled });
    }
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
        _RecreateSwapchain();
        return;
    }

    UpdateUniformBuffer(m_currentFrame, m_currentSwapchainExtent);

    m_device.resetFences(m_inFlightFences[m_currentFrame]);

    m_commandBuffers[m_currentFrame].reset();

    vk::CommandBufferBeginInfo beginInfo {};
    beginInfo.pInheritanceInfo = nullptr;
    m_commandBuffers[m_currentFrame].begin(beginInfo);

    vk::RenderPassBeginInfo renderPassInfo {};
    renderPassInfo.renderPass = m_renderPass;
    renderPassInfo.framebuffer = m_frameBuffers[imageIndex];
    renderPassInfo.renderArea.offset = vk::Offset2D { 0, 0 };
    renderPassInfo.renderArea.extent = m_currentSwapchainExtent;

    std::array<vk::ClearValue, 2> clearValues {};
    clearValues[0].color = vk::ClearColorValue {0.0f, 0.0f, 0.005f, 1.0f};
    clearValues[1].depthStencil = vk::ClearDepthStencilValue { 1.0f, 0 };

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    m_commandBuffers[m_currentFrame].beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
    m_commandBuffers[m_currentFrame].bindPipeline(vk::PipelineBindPoint::eGraphics, m_pipeline);

    vk::Buffer vertexBuffers[] = { m_vertexBuffer.buffer };
    VkDeviceSize offsets[] = { 0 };
    m_commandBuffers[m_currentFrame].bindVertexBuffers(0, 1, vertexBuffers, offsets);
    m_commandBuffers[m_currentFrame].bindIndexBuffer(m_indexBuffer.buffer, 0, vk::IndexType::eUint32);

    vk::Viewport viewport {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(m_currentSwapchainExtent.width);
    viewport.height = static_cast<float>(m_currentSwapchainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    m_commandBuffers[m_currentFrame].setViewport(0, viewport);

    vk::Rect2D scissor {};
    scissor.offset = vk::Offset2D { 0, 0 };
    scissor.extent = m_currentSwapchainExtent;
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
        static_cast<uint32_t>(m_indices.size()),
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
        _RecreateSwapchain();
    }
    m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    m_device.waitIdle();
}

void CVulkanRenderer::UpdateUniformBuffer(uint32_t currentImage, vk::Extent2D swapChainExtent) {
    CUniformBufferObject ubo {};
    ubo.model = glm::mat4(1.0f);
    ubo.view = g_camera.GetViewMatrix();
    ubo.proj = glm::perspective(glm::radians(g_camera.m_fov), swapChainExtent.width / (float)swapChainExtent.height, 0.01f, 50.0f);
    ubo.proj[1][1] *= -1;
    memcpy(m_uniformBuffersData[currentImage], &ubo, sizeof(ubo));
}
