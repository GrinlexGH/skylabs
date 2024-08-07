#include <set>

#include "console.hpp"
#include "vulkanapi.hpp"
#include "SDL.hpp"
#include "SDL_Vulkan.hpp"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

#ifdef NDEBUG
bool enableValidationLayers = false;
#else
extern bool enableValidationLayers;
#endif

const std::vector<const char*> g_validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> g_deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

//============
// CVulkanAPI
CVulkanAPI::~CVulkanAPI() {
    Destroy();
}

void CVulkanAPI::Init(IWindow* window) {
    CreateInstance();
    SetupDebugMessenger();
    CreateSurface(window);
    PickPhysicalDevice();
    CreateLogicalDevice();
    CreateSwapChain(window);

    initialized_ = true;
}

void CVulkanAPI::CreateInstance() {
    VULKAN_HPP_DEFAULT_DISPATCHER.init();

    if (enableValidationLayers && !CheckValidationLayerSupport()) {
        enableValidationLayers = false;
    }

    vk::ApplicationInfo appInfo {
        "Skylabs", VK_MAKE_API_VERSION(0, 0, 0, 0),
        "Skylabs", VK_MAKE_API_VERSION(0, 0, 0, 0),
        VK_API_VERSION_1_3
    };

    vk::InstanceCreateInfo createInfo {};
    createInfo.pApplicationInfo = &appInfo;
    auto requiredExtensions = GetRequiredExtensions();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
    createInfo.ppEnabledExtensionNames = requiredExtensions.data();

    std::vector<vk::ExtensionProperties> availableExtensions =
        vk::enumerateInstanceExtensionProperties();

    for (const auto& requiredExt : requiredExtensions) {
        bool extensionFound = false;
        for (const auto& ext : availableExtensions) {
            if (strcmp(requiredExt, ext.extensionName) == 0) {
                extensionFound = true;
                break;
            }
        }
        if (!extensionFound) {
            throw std::runtime_error(
                std::string("The system doesn't have necessary Vulkan extension: ") + requiredExt + '!'
            );
        }
    }

    vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo {};
    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(g_validationLayers.size());
        createInfo.ppEnabledLayerNames = g_validationLayers.data();
        PopulateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = &debugCreateInfo;
    } else {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
    }

    instance_ = vk::createInstance(createInfo);
    VULKAN_HPP_DEFAULT_DISPATCHER.init(instance_);
}

bool CVulkanAPI::CheckValidationLayerSupport() {
    std::vector<vk::LayerProperties> availableLayers =
        vk::enumerateInstanceLayerProperties();

    for (const auto& requiredLayer : g_validationLayers) {
        bool layerFound = false;
        for (const auto& availableLayer : availableLayers) {
            if (strcmp(requiredLayer, availableLayer.layerName) == 0) {
                layerFound = true;
                break;
            }
        }
        if (!layerFound) {
            return false;
        }
    }
    return true;
}

std::vector<const char*> CVulkanAPI::GetRequiredExtensions() {
    uint32_t extCount = 0;
    SDL::Vulkan::GetRequiredInstanceExtensions(&extCount, nullptr);
    std::vector<const char *> extensions(extCount);
    SDL::Vulkan::GetRequiredInstanceExtensions(&extCount, extensions.data());

    if (enableValidationLayers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
    return extensions;
}

VKAPI_ATTR VkBool32 VKAPI_CALL
DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
              VkDebugUtilsMessageTypeFlagsEXT messageType,
              const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
              void* pUserData
) {
    UNUSED(messageType);
    UNUSED(pUserData);
    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        Error << pCallbackData->pMessage << std::endl;
    } else if (messageSeverity >=
               VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        Warning << pCallbackData->pMessage << std::endl;
    } else {
        Msg << pCallbackData->pMessage << std::endl;
    }
    return VK_FALSE;
}

void CVulkanAPI::PopulateDebugMessengerCreateInfo(vk::DebugUtilsMessengerCreateInfoEXT& createInfo) {
    createInfo.messageSeverity =
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eError |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo;
    createInfo.messageType =
        vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
        vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
        vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
        vk::DebugUtilsMessageTypeFlagBitsEXT::eDeviceAddressBinding;
    createInfo.pfnUserCallback =
        reinterpret_cast<PFN_vkDebugUtilsMessengerCallbackEXT>(DebugCallback);
}

void CVulkanAPI::SetupDebugMessenger() {
    if (!enableValidationLayers)
        return;

    vk::DebugUtilsMessengerCreateInfoEXT createInfo {};
    PopulateDebugMessengerCreateInfo(createInfo);

    debugMessenger_ = instance_.createDebugUtilsMessengerEXT(createInfo);
}

void CVulkanAPI::CreateSurface(IWindow* window) {
    if (!SDL::Vulkan::CreateSurface(
            static_cast<SDL_Window*>(window->GetHandle()),
            instance_,
            &surface_
        )) {
        throw std::runtime_error("Failed to create window surface!");
    }
}

void CVulkanAPI::PickPhysicalDevice() {
    std::vector<vk::PhysicalDevice> devices =
        instance_.enumeratePhysicalDevices();
    if (devices.size() == 0) {
        // todo: automatically change to opengl or dx
        throw std::runtime_error("Failed to find GPUs with Vulkan support!");
    }

    for (const auto& device : devices) {
        // todo: better implementation
        if (isDeviceSuitable(device)) {
            physicalDevice_ = device;
            break;
        }
    }

    if (physicalDevice_ == VK_NULL_HANDLE) {
        throw std::runtime_error("Failed to find a suitable GPU!");
    }
}

bool CVulkanAPI::isDeviceSuitable(vk::PhysicalDevice device) {
    QueueFamilyIndices indices = FindQueueFamilies(device);
    bool extensionsSupported = CheckDeviceExtensionSupport(device);
    bool swapChainAdequate = false;
    if (extensionsSupported) {
        SwapChainSupportDetails swapChainSupport =
            QuerySwapChainSupport(device);
        swapChainAdequate = !swapChainSupport.formats_.empty() &&
                            !swapChainSupport.presentModes_.empty();
    }
    return extensionsSupported && swapChainAdequate && indices.isComplete();
}

CVulkanAPI::QueueFamilyIndices CVulkanAPI::FindQueueFamilies(vk::PhysicalDevice device) const {
    QueueFamilyIndices indices;
    std::vector<vk::QueueFamilyProperties> queueFamilies =
        device.getQueueFamilyProperties();

    int i = 0;
    for (const auto& queueFamily : queueFamilies) {
        if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) {
            indices.graphicsFamily_ = i;
        }
        if (device.getSurfaceSupportKHR(i, surface_)) {
            indices.presentFamily_ = i;
        }
        if (indices.isComplete()) {
            break;
        }
        i++;
    }
    return indices;
}

bool CVulkanAPI::CheckDeviceExtensionSupport(vk::PhysicalDevice device) {
    std::vector<vk::ExtensionProperties> availableExts =
        device.enumerateDeviceExtensionProperties();

    for (const auto& requiredExt : g_deviceExtensions) {
        bool extFound = false;
        for (const auto& availableExt : availableExts) {
            if (strcmp(requiredExt, availableExt.extensionName) == 0) {
                extFound = true;
                break;
            }
        }
        if (!extFound) {
            return false;
        }
    }
    return true;
}

CVulkanAPI::SwapChainSupportDetails CVulkanAPI::QuerySwapChainSupport(vk::PhysicalDevice device) const {
    SwapChainSupportDetails details;
    details.capabilities_   = device.getSurfaceCapabilitiesKHR  (surface_);
    details.formats_        = device.getSurfaceFormatsKHR       (surface_);
    details.presentModes_   = device.getSurfacePresentModesKHR  (surface_);
    return details;
}

void CVulkanAPI::CreateLogicalDevice() {
    if (enableValidationLayers && !CheckDeviceValidationLayerSupport(physicalDevice_)) {
        enableValidationLayers = false;
    }
    QueueFamilyIndices indices = FindQueueFamilies(physicalDevice_);

    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily_.value(),
                                               indices.presentFamily_.value() };

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
    createInfo.queueCreateInfoCount =
        static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount =
        static_cast<uint32_t>(g_deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = g_deviceExtensions.data();
    if (enableValidationLayers) {
        createInfo.enabledLayerCount =
            static_cast<uint32_t>(g_validationLayers.size());
        createInfo.ppEnabledLayerNames = g_validationLayers.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }

    device_         = physicalDevice_.createDevice(createInfo);
    graphicsQueue_  = device_.getQueue(indices.graphicsFamily_.value(), 0);
    presentQueue_   = device_.getQueue(indices.presentFamily_.value(),  0);
}

bool CVulkanAPI::CheckDeviceValidationLayerSupport(vk::PhysicalDevice device) {
    std::vector<vk::LayerProperties> availableLayers =
        device.enumerateDeviceLayerProperties();

    for (const auto& requiredLayer : g_validationLayers) {
        bool layerFound = false;
        for (const auto& availableLayer : availableLayers) {
            if (strcmp(requiredLayer, availableLayer.layerName) == 0) {
                layerFound = true;
                break;
            }
        }
        if (!layerFound) {
            return false;
        }
    }
    return true;
}

void CVulkanAPI::CreateSwapChain(IWindow* window) {
    SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(physicalDevice_);

    vk::SurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.formats_);
    vk::PresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.presentModes_);
    vk::Extent2D extent = ChooseSwapExtent(window, swapChainSupport.capabilities_);

    uint32_t imageCount = swapChainSupport.capabilities_.minImageCount + 1;

    if (swapChainSupport.capabilities_.maxImageCount > 0 &&
        imageCount > swapChainSupport.capabilities_.maxImageCount
    ) {
        imageCount = swapChainSupport.capabilities_.maxImageCount;
    }

    vk::SwapchainCreateInfoKHR createInfo{};
    createInfo.surface = surface_;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;

    QueueFamilyIndices indices = FindQueueFamilies(physicalDevice_);
    uint32_t queueFamilyIndices[] = { indices.graphicsFamily_.value(), indices.presentFamily_.value() };

    if (indices.graphicsFamily_ != indices.presentFamily_) {
        createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = vk::SharingMode::eExclusive;
    }

    createInfo.preTransform = swapChainSupport.capabilities_.currentTransform;
    createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    createInfo.presentMode = presentMode;
    createInfo.clipped = vk::True;
    createInfo.oldSwapchain = VK_NULL_HANDLE;
    swapChain_ = device_.createSwapchainKHR(createInfo);

    swapChainImages_ = device_.getSwapchainImagesKHR(swapChain_);
    swapChainImageFormat_ = surfaceFormat.format;
    swapChainExtent_ = extent;
}

vk::SurfaceFormatKHR CVulkanAPI::ChooseSwapSurfaceFormat(
    const std::vector<vk::SurfaceFormatKHR>& availableFormats
) {
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == vk::Format::eR8G8B8A8Srgb &&
            availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear
        ) {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

vk::PresentModeKHR CVulkanAPI::ChooseSwapPresentMode(
    const std::vector<vk::PresentModeKHR>& availablePresentModes
) {
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == vk::PresentModeKHR::eImmediate) { // !change
            return availablePresentMode;
        }
    }

    return vk::PresentModeKHR::eImmediate;
}
vk::Extent2D CVulkanAPI::ChooseSwapExtent(
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

void CVulkanAPI::CreateImageViews() {
    swapChainImageViews_.resize(swapChainImages_.size());
    for (size_t i = 0; i < swapChainImages_.size(); i++) {
        vk::ImageViewCreateInfo createInfo {};
        createInfo.image = swapChainImages_[i];
        createInfo.viewType = vk::ImageViewType::e2D;
        createInfo.format = swapChainImageFormat_;
        createInfo.components.r = vk::ComponentSwizzle::eIdentity;
        createInfo.components.g = vk::ComponentSwizzle::eIdentity;
        createInfo.components.b = vk::ComponentSwizzle::eIdentity;
        createInfo.components.a = vk::ComponentSwizzle::eIdentity;
        createInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;
        swapChainImageViews_[i] = device_.createImageView(createInfo);
    }
}

void CVulkanAPI::Destroy() {
    if(!initialized_)
        return;

    for (auto imageView : swapChainImageViews_) {
        vkDestroyImageView(device_, imageView, nullptr);
    }
    device_.destroySwapchainKHR();
    device_.destroy();
    instance_.destroySurfaceKHR(surface_);
    if (enableValidationLayers) {
        instance_.destroyDebugUtilsMessengerEXT(debugMessenger_);
    }
    instance_.destroy();
}
