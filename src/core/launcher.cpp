#include "libraries.hpp"

#include <memory>

#include "launcher.hpp"

static SDL_Window *window;
vk::Instance instance;

static void cleanup() {
    vkDestroyInstance(instance, nullptr);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

static void initWindow()
{
    SDL_Init(SDL_INIT_VIDEO);
    window =
        SDL_CreateWindow("Skylabs", SDL_WINDOWPOS_CENTERED,
                         SDL_WINDOWPOS_CENTERED, 640, 480, SDL_WINDOW_VULKAN);
}

static void createInstance() {
    vk::ApplicationInfo appInfo{ "Skylabs", VK_MAKE_API_VERSION(0, 0, 0, 0),
                                "Skylabs", VK_MAKE_API_VERSION(0, 0, 0, 0),
                                VK_API_VERSION_1_3 };

    vk::InstanceCreateInfo createInfo{};
    createInfo.pApplicationInfo = &appInfo;

    uint32_t extensionCount;
    SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, nullptr);
    auto extensionNames = std::make_unique<const char *[]>(extensionCount);
    SDL_Vulkan_GetInstanceExtensions(window, &extensionCount,
                                     extensionNames.get());

    createInfo.enabledExtensionCount = extensionCount;
    createInfo.ppEnabledExtensionNames = extensionNames.get();
    createInfo.enabledLayerCount = 0;

    std::vector<vk::ExtensionProperties> availableExtensions =
        vk::enumerateInstanceExtensionProperties();

    std::vector<vk::ExtensionProperties> neededExtensions;
    for (uint32_t i = 0; i < extensionCount; ++i) {
        neededExtensions.emplace_back(*(extensionNames.get() + i));
    }

    for (const auto &neededExtension : neededExtensions) {
        if (std::ranges::find(availableExtensions, neededExtension) ==
            availableExtensions.end()) {
            throw std::runtime_error(
                "The system doesn't have necessary vulkan extensions!");
        }
    }

    try {
        instance = vk::createInstance(createInfo);
    } catch (const std::exception &e) {
        cleanup();
        throw e;
    }
}

static void initVulkan() { createInstance(); }

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

void CLauncher::Main() {
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
    const std::vector<const char *> validationLayers = {
        "VK_LAYER_KHRONOS_validation" };

    std::vector<vk::LayerProperties> availableLayers =
        vk::enumerateInstanceLayerProperties();

    for (const char *layerName : validationLayers) {
        bool layerFound = false;
        for (const auto &layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }
        if (!layerFound) {
            throw std::runtime_error("pizdui kurva");
        }
  }
}
