#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include "vulkan_initializer.hpp"

#include "SDL.hpp"
#include "SDL_Vulkan.hpp"
#include "console.hpp"

#include <set>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace VulkanInitializer {
#ifdef NDEBUG
    bool enableValidationLayers = false;
#else
    bool enableValidationLayers = true;
#endif
}

VKAPI_ATTR vk::Bool32 VKAPI_CALL VulkanInitializer::DebugCallback(
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

std::vector<std::string_view> VulkanInitializer::FindMissingLayers(
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

std::vector<const char*> VulkanInitializer::GetRequiredInstanceExtensions() {
    uint32_t extCount = 0;
    SDL::Vulkan::GetRequiredInstanceExtensions(&extCount, nullptr);
    std::vector<const char*> extensions(extCount);
    SDL::Vulkan::GetRequiredInstanceExtensions(&extCount, extensions.data());

    if (enableValidationLayers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
    return extensions;
}

std::vector<std::string_view> VulkanInitializer::FindMissingExtensions(
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

bool VulkanInitializer::isDeviceSuitable(vk::PhysicalDevice device, vk::SurfaceKHR surface) {
    CQueueFamilyIndices indices = FindQueueFamilies(device, surface);
    bool extensionsSupported = FindMissingExtensions(device.enumerateDeviceExtensionProperties(), g_deviceExtensions).empty();
    bool swapChainAdequate = false;
    if (extensionsSupported) {
        CSwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(device, surface);
        swapChainAdequate = !swapChainSupport.m_formats.empty() &&
                            !swapChainSupport.m_presentModes.empty();
    }
    return extensionsSupported && swapChainAdequate && indices.isComplete();
}

VulkanInitializer::CQueueFamilyIndices VulkanInitializer::FindQueueFamilies(vk::PhysicalDevice device, vk::SurfaceKHR surface) {
    CQueueFamilyIndices indices;
    std::vector<vk::QueueFamilyProperties> queueFamilies = device.getQueueFamilyProperties();

    int i = 0;
    for (const auto& queueFamily : queueFamilies) {
        if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) {
            indices.m_graphicsFamily = i;
        }
        if (device.getSurfaceSupportKHR(i, surface)) {
            indices.m_presentFamily = i;
        }
        if (indices.isComplete()) {
            break;
        }
        i++;
    }
    return indices;
}

VulkanInitializer::CSwapChainSupportDetails VulkanInitializer::QuerySwapChainSupport(vk::PhysicalDevice device, vk::SurfaceKHR surface) {
    CSwapChainSupportDetails details;
    details.m_capabilities = device.getSurfaceCapabilitiesKHR(surface);
    details.m_formats = device.getSurfaceFormatsKHR(surface);
    details.m_presentModes = device.getSurfacePresentModesKHR(surface);
    return details;
}

vk::Instance VulkanInitializer::CreateInstance(vk::DebugUtilsMessengerEXT& debugMessenger) {
    vk::Instance instance;
    VULKAN_HPP_DEFAULT_DISPATCHER.init();

    if (enableValidationLayers && !FindMissingLayers(vk::enumerateInstanceLayerProperties(), g_validationLayers).empty()) {
        enableValidationLayers = false;
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

    vk::DebugUtilsMessengerCreateInfoEXT debugMessengerCI;
    if (enableValidationLayers) {
        debugMessengerCI.messageSeverity =
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eError |
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo;
        debugMessengerCI.messageType =
            vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
            vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
            vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
            vk::DebugUtilsMessageTypeFlagBitsEXT::eDeviceAddressBinding;
        debugMessengerCI.pfnUserCallback =
            reinterpret_cast<PFN_vkDebugUtilsMessengerCallbackEXT>(DebugCallback);
        createInfo.enabledLayerCount = static_cast<uint32_t>(g_validationLayers.size());
        createInfo.ppEnabledLayerNames = g_validationLayers.data();
        createInfo.pNext = &debugMessengerCI;
    } else {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
    }

    instance = vk::createInstance(createInfo);
    VULKAN_HPP_DEFAULT_DISPATCHER.init(instance);
    if (enableValidationLayers) {
        debugMessenger = instance.createDebugUtilsMessengerEXT(debugMessengerCI);
    }
    return instance;
}

vk::SurfaceFormatKHR VulkanInitializer::ChooseSwapSurfaceFormat(
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

vk::PresentModeKHR VulkanInitializer::ChooseSwapPresentMode(
    const std::vector<vk::PresentModeKHR>& availablePresentModes
) {
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == vk::PresentModeKHR::eFifo) { // !change
            return availablePresentMode;
        }
    }

    return vk::PresentModeKHR::eImmediate;
}

vk::Extent2D VulkanInitializer::ChooseSwapExtent(
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

vk::SurfaceKHR VulkanInitializer::CreateSurface(vk::Instance instance, IWindow* window) {
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

vk::PhysicalDevice VulkanInitializer::PickPhysicalDevice(vk::Instance instance, vk::SurfaceKHR surface) {
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

vk::Device VulkanInitializer::CreateLogicalDevice(
    vk::PhysicalDevice physicalDevice,
    vk::SurfaceKHR surface,
    vk::Queue& graphicsQueue,
    vk::Queue& presentQueue
) {
    if (enableValidationLayers && FindMissingLayers(physicalDevice.enumerateDeviceLayerProperties(), g_validationLayers).empty()) {
        enableValidationLayers = false;
    }
    CQueueFamilyIndices indices = FindQueueFamilies(physicalDevice, surface);

    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {
        indices.m_graphicsFamily.value(),
        indices.m_presentFamily.value()
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
    graphicsQueue = device.getQueue(indices.m_graphicsFamily.value(), 0);
    presentQueue = device.getQueue(indices.m_presentFamily.value(), 0);
    return device;
}

vk::SwapchainKHR VulkanInitializer::CreateSwapChain(
    vk::PhysicalDevice physicalDevice,
    vk::Device device,
    vk::SurfaceKHR surface,
    IWindow* window,
    std::vector<vk::Image>& swapChainImages,
    vk::Format& swapChainImageFormat,
    vk::Extent2D& swapChainExtent
) {
    CSwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(physicalDevice, surface);

    vk::SurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.m_formats);
    vk::PresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.m_presentModes);
    vk::Extent2D extent = ChooseSwapExtent(window, swapChainSupport.m_capabilities);

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

    CQueueFamilyIndices indices = FindQueueFamilies(physicalDevice, surface);
    uint32_t queueFamilyIndices[] = { indices.m_graphicsFamily.value(), indices.m_presentFamily.value() };

    if (indices.m_graphicsFamily != indices.m_presentFamily) {
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
    vk::SwapchainKHR swapChain {};
    swapChain = device.createSwapchainKHR(createInfo);

    swapChainImages = device.getSwapchainImagesKHR(swapChain);
    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = extent;

    return swapChain;
}

std::vector<vk::ImageView> VulkanInitializer::CreateImageViews(
    vk::Device device,
    const std::vector<vk::Image>& swapChainImages,
    vk::Format swapChainImageFormat
) {
    std::vector<vk::ImageView> swapChainImageViews {};
    swapChainImageViews.resize(swapChainImages.size());
    for (std::size_t i = 0; i < swapChainImages.size(); i++) {
        vk::ImageViewCreateInfo createInfo {};
        createInfo.image = swapChainImages[i];
        createInfo.viewType = vk::ImageViewType::e2D;
        createInfo.format = swapChainImageFormat;
        createInfo.components.r = vk::ComponentSwizzle::eIdentity;
        createInfo.components.g = vk::ComponentSwizzle::eIdentity;
        createInfo.components.b = vk::ComponentSwizzle::eIdentity;
        createInfo.components.a = vk::ComponentSwizzle::eIdentity;
        createInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;
        swapChainImageViews[i] = device.createImageView(createInfo);
    }
    return swapChainImageViews;
}
