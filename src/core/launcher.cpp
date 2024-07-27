#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#include <iostream>
#include <memory>
#include <optional>

#include "console.hpp"
#include "launcher.hpp"

static SDL_Window* g_window;
static vk::Instance g_instance;
static vk::DebugUtilsMessengerEXT g_debugMessenger;
static vk::PhysicalDevice g_physicalDevice;
static vk::Device g_device;
static vk::Queue g_graphicsQueue;

#ifdef NDEBUG
static constexpr bool enableValidationLayers = false;
#else
static constexpr bool enableValidationLayers = true;
#endif

static const std::vector<const char*> g_validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

void cleanup() {
    if (enableValidationLayers) {
        g_instance.destroyDebugUtilsMessengerEXT(g_debugMessenger);
    }
    g_device.destroy();
    g_instance.destroy();
    SDL_DestroyWindow(g_window);
    SDL_Quit();
}

static void mainLoop() {
    bool quit = false;
    while (!quit) {
        SDL_Event e;
        SDL_WaitEvent(&e);
        if (e.type == SDL_QUIT) {
            quit = true;
        }
    }
}

#pragma region picking physical device
struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    bool isComplete() const { return graphicsFamily.has_value(); }
};

static QueueFamilyIndices findQueueFamilies(vk::PhysicalDevice device) {
    QueueFamilyIndices indices;
    std::vector<vk::QueueFamilyProperties> queueFamilies =
        device.getQueueFamilyProperties();

    int i = 0;
    for (const auto& queueFamily : queueFamilies) {
        if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) {
            indices.graphicsFamily = i;
        }
        if (indices.isComplete()) {
            break;
        }
        i++;
    }
    return indices;
}

static bool isDeviceSuitable(vk::PhysicalDevice device) {
    // todo: better implementation
    QueueFamilyIndices indices = findQueueFamilies(device);
    return indices.isComplete();
}

static void pickPhysicalDevice() {
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
#pragma endregion

#pragma region creating logical device and queue
static bool checkDeviceValidationLayerSupport(vk::PhysicalDevice device) {
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

static void createLogicalDevice() {
    if (enableValidationLayers &&
        !checkDeviceValidationLayerSupport(g_physicalDevice)) {
        throw std::runtime_error(
            "validation layers requested, but not available!");
    }
    QueueFamilyIndices indices = findQueueFamilies(g_physicalDevice);

    vk::DeviceQueueCreateInfo queueCreateInfo {};
    queueCreateInfo.queueFamilyIndex = indices.graphicsFamily.value();
    queueCreateInfo.queueCount = 1;
    float queuePriority = 1.0f;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    vk::PhysicalDeviceFeatures deviceFeatures {};

    vk::DeviceCreateInfo createInfo {};
    createInfo.pQueueCreateInfos = &queueCreateInfo;
    createInfo.queueCreateInfoCount = 1;
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = 0;
    if (enableValidationLayers) {
        createInfo.enabledLayerCount =
            static_cast<uint32_t>(g_validationLayers.size());
        createInfo.ppEnabledLayerNames = g_validationLayers.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }

    g_device = g_physicalDevice.createDevice(createInfo);
    g_graphicsQueue = g_device.getQueue(indices.graphicsFamily.value(), 0);
}
#pragma endregion

#pragma region validation layer setup
static VKAPI_ATTR VkBool32 VKAPI_CALL
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

static void populateDebugMessengerCreateInfo(
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
    createInfo.pfnUserCallback = debugCallback;
}

static bool checkValidationLayerSupport() {
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

static void setupDebugMessenger() {
    if (!enableValidationLayers)
        return;
    vk::DebugUtilsMessengerCreateInfoEXT createInfo {};
    populateDebugMessengerCreateInfo(createInfo);

    g_debugMessenger = g_instance.createDebugUtilsMessengerEXT(createInfo);
}
#pragma endregion

#pragma region initializing vulkan and window
static std::vector<const char*> getRequiredExtensions() {
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

static void createInstance() {
    VULKAN_HPP_DEFAULT_DISPATCHER.init();

    if (!checkValidationLayerSupport() && enableValidationLayers) {
        throw std::runtime_error(
            "validation layers requested, but not available!");
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
        createInfo.pNext =
            (vk::DebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
    } else {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
    }

    g_instance = vk::createInstance(createInfo);
    VULKAN_HPP_DEFAULT_DISPATCHER.init(g_instance);
}

static void initWindow() {
    // TODO: something may happened wrong here
    SDL_Init(SDL_INIT_VIDEO);
    g_window =
        SDL_CreateWindow("Skylabs", SDL_WINDOWPOS_CENTERED,
                         SDL_WINDOWPOS_CENTERED, 640, 480, SDL_WINDOW_VULKAN);
}
#pragma endregion

static void initVulkan() {
    createInstance();
    setupDebugMessenger();
    pickPhysicalDevice();
    createLogicalDevice();
}

void CLauncher::Main() {
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
}
