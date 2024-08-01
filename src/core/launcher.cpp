#include <stdexcept>
#include "vulkanapi.hpp"
#include <SDL.h>
#include <SDL_vulkan.h>

#include <iostream>
#include <memory>
#include <optional>
#include <set>

#include "console.hpp"
#include "launcher.hpp"

SDL_Window* g_window;
vk::Instance g_instance;
vk::DebugUtilsMessengerEXT g_debugMessenger;
vk::SurfaceKHR g_surface;
vk::PhysicalDevice g_physicalDevice;
vk::Device g_device;
vk::Queue g_graphicsQueue;
vk::Queue g_presentQueue;

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

void cleanup() {
    if (enableValidationLayers) {
        g_instance.destroyDebugUtilsMessengerEXT(g_debugMessenger);
    }
    g_device.destroy();
    g_instance.destroySurfaceKHR(g_surface);
    g_instance.destroy();
    SDL_DestroyWindow(g_window);
    SDL_Quit();
}

void mainLoop() {
    bool quit = false;
    while (!quit) {
        SDL_Event e;
        SDL_WaitEvent(&e);
        if (e.type == SDL_QUIT) {
            quit = true;
        }
    }
}

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

QueueFamilyIndices findQueueFamilies(vk::PhysicalDevice device) {
    QueueFamilyIndices indices;
    std::vector<vk::QueueFamilyProperties> queueFamilies =
        device.getQueueFamilyProperties();

    int i = 0;
    for (const auto& queueFamily : queueFamilies) {
        if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) {
            indices.graphicsFamily = i;
        }
        if (device.getSurfaceSupportKHR(i, g_surface)) {
            indices.presentFamily = i;
        }
        if (indices.isComplete()) {
            break;
        }
        i++;
    }
    return indices;
}

struct SwapChainSupportDetails {
    vk::SurfaceCapabilitiesKHR capabilities;
    std::vector<vk::SurfaceFormatKHR> formats;
    std::vector<vk::PresentModeKHR> presentModes;
};

SwapChainSupportDetails querySwapChainSupport(vk::PhysicalDevice device) {
    SwapChainSupportDetails details;
    details.capabilities = device.getSurfaceCapabilitiesKHR(g_surface);
    details.formats = device.getSurfaceFormatsKHR(g_surface);
    details.presentModes = device.getSurfacePresentModesKHR(g_surface);
    return details;
}

bool checkDeviceExtensionSupport(vk::PhysicalDevice device) {
    std::vector<vk::ExtensionProperties> availableExtensions =
        device.enumerateDeviceExtensionProperties();

    std::set<std::string> requiredExtensions(g_deviceExtensions.begin(),
                                             g_deviceExtensions.end());

    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

bool isDeviceSuitable(vk::PhysicalDevice device) {
    // todo: better implementation
    QueueFamilyIndices indices = findQueueFamilies(device);
    bool extensionsSupported = checkDeviceExtensionSupport(device);
    bool swapChainAdequate = false;
    if (extensionsSupported) {
        SwapChainSupportDetails swapChainSupport =
            querySwapChainSupport(device);
        swapChainAdequate = !swapChainSupport.formats.empty() &&
                            !swapChainSupport.presentModes.empty();
    }
    return extensionsSupported && swapChainAdequate && indices.isComplete();
}

void createSurface() {
    if (!SDL_Vulkan_CreateSurface(
            g_window, g_instance,
            reinterpret_cast<VkSurfaceKHR*>(&g_surface))) {
        throw std::runtime_error("failed to create window surface!");
    }
}

vk::SurfaceFormatKHR chooseSwapSurfaceFormat(
    const std::vector<vk::SurfaceFormatKHR>& availableFormats) {
    UNUSED(availableFormats);
    return {};
}

void pickPhysicalDevice() {
    std::vector<vk::PhysicalDevice> devices =
        g_instance.enumeratePhysicalDevices();
    if (devices.size() == 0) {
        // todo: automatically change to opengl or dx
        throw std::runtime_error("Failed to find GPUs with Vulkan support!");
    }

    for (const auto& device : devices) {
        if (isDeviceSuitable(device)) {
            g_physicalDevice = device;
            break;
        }
    }

    if (g_physicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("Failed to find a suitable GPU!");
    }
}

bool checkDeviceValidationLayerSupport(vk::PhysicalDevice device) {
    std::vector<vk::LayerProperties> availableLayers =
        device.enumerateDeviceLayerProperties();

    for (const auto& layerName : g_validationLayers) {
        bool layerFound = false;
        for (const auto& Layer : availableLayers) {
            if (strcmp(layerName, Layer.layerName) == 0) {
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

void createLogicalDevice() {
    if (!checkDeviceValidationLayerSupport(g_physicalDevice)) {
        enableValidationLayers = false;
    }
    QueueFamilyIndices indices = findQueueFamilies(g_physicalDevice);

    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(),
                                               indices.presentFamily.value() };

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

    g_device = g_physicalDevice.createDevice(createInfo);
    g_graphicsQueue = g_device.getQueue(indices.graphicsFamily.value(), 0);
    g_presentQueue = g_device.getQueue(indices.presentFamily.value(), 0);
}

VKAPI_ATTR VkBool32 VKAPI_CALL
debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
              VkDebugUtilsMessageTypeFlagsEXT messageType,
              const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
              void* pUserData) {
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

void populateDebugMessengerCreateInfo(
    vk::DebugUtilsMessengerCreateInfoEXT& createInfo) {
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
        reinterpret_cast<PFN_vkDebugUtilsMessengerCallbackEXT>(debugCallback);
}

bool checkValidationLayerSupport() {
    std::vector<vk::LayerProperties> availableLayers =
        vk::enumerateInstanceLayerProperties();

    for (const auto& layerName : g_validationLayers) {
        bool layerFound = false;
        for (const auto& Layer : availableLayers) {
            if (strcmp(layerName, Layer.layerName) == 0) {
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

void setupDebugMessenger() {
    if (!enableValidationLayers)
        return;
    vk::DebugUtilsMessengerCreateInfoEXT createInfo {};
    populateDebugMessengerCreateInfo(createInfo);

    g_debugMessenger = g_instance.createDebugUtilsMessengerEXT(createInfo);
}

std::vector<const char*> getRequiredExtensions() {
    uint32_t extCount = 0;
    SDL_Vulkan_GetInstanceExtensions(g_window, &extCount, nullptr);
    auto extNames = std::make_unique<const char*[]>(extCount);
    SDL_Vulkan_GetInstanceExtensions(g_window, &extCount, extNames.get());

    std::vector<const char*> extensions(extNames.get(),
                                        extNames.get() + extCount);

    if (enableValidationLayers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}

void createInstance() {
    VULKAN_HPP_DEFAULT_DISPATCHER.init();

    if (enableValidationLayers && !checkValidationLayerSupport()) {
        enableValidationLayers = false;
    }

    vk::ApplicationInfo appInfo { "Skylabs", VK_MAKE_API_VERSION(0, 0, 0, 0),
                                  "Skylabs", VK_MAKE_API_VERSION(0, 0, 0, 0),
                                  VK_API_VERSION_1_3 };

    vk::InstanceCreateInfo createInfo {};
    createInfo.pApplicationInfo = &appInfo;
    auto requiredExtensions = getRequiredExtensions();
    createInfo.enabledExtensionCount =
        static_cast<uint32_t>(requiredExtensions.size());
    createInfo.ppEnabledExtensionNames = requiredExtensions.data();

    std::vector<vk::ExtensionProperties> availableExtensions =
        vk::enumerateInstanceExtensionProperties();

    for (const auto& neededExtension : requiredExtensions) {
        bool extensionFound = false;
        for (const auto& extension : availableExtensions) {
            if (strcmp(neededExtension, extension.extensionName) == 0) {
                extensionFound = true;
                break;
            }
        }
        if (!extensionFound) {
            throw std::runtime_error(
                "The system doesn't have necessary vulkan extensions!");
        }
    }

    vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo {};
    if (enableValidationLayers) {
        createInfo.enabledLayerCount =
            static_cast<uint32_t>(g_validationLayers.size());
        createInfo.ppEnabledLayerNames = g_validationLayers.data();

        populateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = &debugCreateInfo;
    } else {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
    }

    g_instance = vk::createInstance(createInfo);
    VULKAN_HPP_DEFAULT_DISPATCHER.init(g_instance);
}

void initWindow() {
    // TODO: something may happened wrong here
    SDL_Init(SDL_INIT_VIDEO);
    g_window =
        SDL_CreateWindow("Skylabs", SDL_WINDOWPOS_CENTERED,
                         SDL_WINDOWPOS_CENTERED, 640, 480, SDL_WINDOW_VULKAN);
}

void initVulkan() {
    createInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
}

void CLauncher::Main() {
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
}
