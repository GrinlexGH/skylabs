#include <set>

#include "console.hpp"
#include "vulkanapi.hpp"
#include "SDL.hpp"
#include "SDL_Vulkan.hpp"

#include "vulkan_initializer.hpp"

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

//============
// CVulkanAPI
CVulkanAPI::~CVulkanAPI() {
    Destroy();
}

void CVulkanAPI::Init(IWindow* window) {
    using namespace VulkanInitializer;

    if (!window->GetHandle()) {
        throw std::runtime_error("Cant initialize vulkan: window is nullptr!\n");
    }

    instance_ = CreateInstance(debugMessenger_);
    surface_ = CreateSurface(instance_, window);
    physicalDevice_ = PickPhysicalDevice(instance_, surface_);
    device_ = CreateLogicalDevice(physicalDevice_, surface_, graphicsQueue_, presentQueue_);
    swapChain_ = CreateSwapChain(
        physicalDevice_, device_,
        surface_, window,
        swapChainImages_, swapChainImageFormat_, swapChainExtent_
    );
    swapChainImageViews_ = CreateImageViews(device_, swapChainImages_, swapChainImageFormat_);
    initialized_ = true;
}

void CVulkanAPI::Destroy() {
    if (!initialized_)
        return;

    for (const auto& imageView : swapChainImageViews_) {
        device_.destroyImageView(imageView);
    }

    device_.destroySwapchainKHR(swapChain_);
    device_.destroy();
    instance_.destroySurfaceKHR(surface_);
    if (enableValidationLayers) {
        instance_.destroyDebugUtilsMessengerEXT(debugMessenger_);
    }
    instance_.destroy();
}
