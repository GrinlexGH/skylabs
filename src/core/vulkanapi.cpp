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
    CQueueFamilyIndices queueIndices = FindQueueFamilies(m_physicalDevice, m_surface);

    m_device = CreateLogicalDevice(m_physicalDevice, queueIndices);
    m_graphicsQueue = m_device.getQueue(queueIndices.m_graphicsFamily.value(), 0);
    m_presentQueue = m_device.getQueue(queueIndices.m_presentFamily.value(), 0);

    CSwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(m_physicalDevice, m_surface);
    vk::SurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.m_formats);
    vk::PresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.m_presentModes);
    m_swapChainExtent = ChooseSwapExtent(window, swapChainSupport.m_capabilities);

    m_swapChain = CreateSwapChain(
        m_device, m_surface,
        queueIndices, swapChainSupport,
        surfaceFormat, presentMode, m_swapChainExtent
    );

    m_swapChainImages = m_device.getSwapchainImagesKHR(m_swapChain);
    m_swapChainImageFormat = surfaceFormat.format;

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
