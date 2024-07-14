#include "launcher.hpp"

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.hpp>
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#include <iostream>
#include <memory>

static SDL_Window *g_window;
static vk::Instance g_instance;
static vk::DebugUtilsMessengerEXT g_debugMessenger;

#ifdef NDEBUG
static constexpr bool enableValidationLayers = false;
#else
static constexpr bool enableValidationLayers = true;
#endif

static std::vector<const char *> g_validationLayers = {
    "VK_LAYER_KHRONOS_validation"};

static void cleanup() {
    if (enableValidationLayers) {
        g_instance.destroyDebugUtilsMessengerEXT();
    }
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

static VKAPI_ATTR VkBool32 VKAPI_CALL
debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
              VkDebugUtilsMessageTypeFlagsEXT messageType,
              const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
              void *pUserData) {
    (void)messageSeverity;
    (void)messageType;
    (void)pUserData;
    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
}

static void setupDebugMessenger() {
    if (!enableValidationLayers)
        return;
    vk::DebugUtilsMessengerCreateInfoEXT createInfo{};
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

    g_debugMessenger = g_instance.createDebugUtilsMessengerEXT(createInfo);
}

std::vector<const char *> getRequiredExtensions() {
    uint32_t extCount = 0;
    SDL_Vulkan_GetInstanceExtensions(g_window, &extCount, nullptr);
    auto extNames = std::make_unique<const char *[]>(extCount);
    SDL_Vulkan_GetInstanceExtensions(g_window, &extCount, extNames.get());

    std::vector<const char *> extensions(extNames.get(),
                                         extNames.get() + extCount);

    if (enableValidationLayers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}

static bool checkValidationLayerSupport() {
    std::vector<vk::LayerProperties> availableLayers =
        vk::enumerateInstanceLayerProperties();

    for (const auto &layerName : g_validationLayers) {
        bool layerFound = false;
        for (const auto &Layer : availableLayers) {
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

static void createInstance() {
    VULKAN_HPP_DEFAULT_DISPATCHER.init();
    if (enableValidationLayers && !checkValidationLayerSupport()) {
        throw std::runtime_error(
            "validation layers requested, but not available!");
    }

    vk::ApplicationInfo appInfo{"Skylabs", VK_MAKE_API_VERSION(0, 0, 0, 0),
                                "Skylabs", VK_MAKE_API_VERSION(0, 0, 0, 0),
                                VK_API_VERSION_1_3};

    vk::InstanceCreateInfo createInfo{};

    createInfo.pApplicationInfo = &appInfo;
    auto requiredExtensions = getRequiredExtensions();
    createInfo.enabledExtensionCount =
        static_cast<uint32_t>(requiredExtensions.size());
    createInfo.ppEnabledExtensionNames = requiredExtensions.data();

    std::vector<vk::ExtensionProperties> availableExtensions =
        vk::enumerateInstanceExtensionProperties();

    for (const auto &neededExtension : requiredExtensions) {
        bool extensionFound = false;
        for (const auto &extension : availableExtensions) {
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

    vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if (enableValidationLayers) {
        createInfo.enabledLayerCount =
            static_cast<uint32_t>(g_validationLayers.size());
        createInfo.ppEnabledLayerNames = g_validationLayers.data();

        debugCreateInfo.messageSeverity =
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eError |
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo;
        debugCreateInfo.messageType =
            vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
            vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
            vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
            vk::DebugUtilsMessageTypeFlagBitsEXT::eDeviceAddressBinding;
        debugCreateInfo.pfnUserCallback = debugCallback;
        createInfo.pNext =
            (vk::DebugUtilsMessengerCreateInfoEXT *)&debugCreateInfo;
    } else {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
    }

    g_instance = vk::createInstance(createInfo);
    VULKAN_HPP_DEFAULT_DISPATCHER.init(g_instance);
}

static void initVulkan() {
    createInstance();
    setupDebugMessenger();
}

static void initWindow() {
    // TODO: something may happened wrong here
    SDL_Init(SDL_INIT_VIDEO);
    g_window =
        SDL_CreateWindow("Skylabs", SDL_WINDOWPOS_CENTERED,
                         SDL_WINDOWPOS_CENTERED, 640, 480, SDL_WINDOW_VULKAN);
}

void CLauncher::Main() {
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
}
