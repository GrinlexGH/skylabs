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

    m_instance = CreateInstance(m_debugMessenger);
    m_surface = CreateSurface(m_instance, window);
    m_physicalDevice = PickPhysicalDevice(m_instance, m_surface);
    m_device = CreateLogicalDevice(m_physicalDevice, m_surface, m_graphicsQueue, m_presentQueue);
    m_swapChain = CreateSwapChain(
        m_physicalDevice, m_device,
        m_surface, window,
        m_swapChainImages, m_swapChainImageFormat, m_swapChainExtent
    );
    m_swapChainImageViews = CreateImageViews(m_device, m_swapChainImages, m_swapChainImageFormat);
    m_initialized = true;
}

void CVulkanAPI::Destroy() {
    if (!m_initialized)
        return;

    for (const auto& imageView : m_swapChainImageViews) {
        m_device.destroyImageView(imageView);
    }

    m_device.destroySwapchainKHR(m_swapChain);
    m_device.destroy();
    m_instance.destroySurfaceKHR(m_surface);
    if (enableValidationLayers) {
        m_instance.destroyDebugUtilsMessengerEXT(m_debugMessenger);
    }
    m_instance.destroy();
}
