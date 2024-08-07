#include <stdexcept>
#include "vulkanapi.hpp"
#include "SDL.hpp"
#include <SDL_vulkan.h>

#include <iostream>
#include <memory>
#include <optional>
#include <set>

#include "console.hpp"
#include "launcher.hpp"

SDL::CWindow g_window;
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

void CLauncher::Main() {
    SDL::Handle sdlHandle;
    g_window.Create(
        "Skylabs",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        640, 480,
        SDL_WINDOW_VULKAN
    );
    CVulkanAPI vulkan;
    vulkan.Init(&g_window);
    mainLoop();
}
