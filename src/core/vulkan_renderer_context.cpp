#include "vulkan_renderer_context.hpp"

#include "SDL_Vulkan.hpp"
#include "console.hpp"

#include <ranges>

extern bool g_enableValidationLayers;

//============
// CVulkanRendererContext
void CVulkanRendererContext::Initialize(IWindow* window) {
    if (!window->GetHandle()) {
        throw std::runtime_error("Can't initialize vulkan renderer context: window is nullptr!\n");
    }

    m_window = window;

    InitializeInstanceExtensions();
    InitializeInstance();

    m_surface = CreateSurface();
}

void CVulkanRendererContext::InitializeInstanceExtensions() {
    VULKAN_HPP_DEFAULT_DISPATCHER.init();

    uint32_t extCount = 0;
    const char* const* requiredExtensions = nullptr;

    // Register required platform extensions
    if (m_window->GetVendor() == WindowVendor::eSDL) {
        requiredExtensions = SDL::Vulkan::GetRequiredInstanceExtensions(&extCount);
    } else {
        assert(false && "Nothing except sdl is implemented yet."); // TODO
    }

    if (requiredExtensions) {
        for (uint32_t i = 0; i < extCount; ++i) {
            m_requestedInstanceExtensions[requiredExtensions[i]] = true;
        }
    }

    // Optional debug utils
    if (g_enableValidationLayers) {
        m_requestedInstanceExtensions[VK_EXT_DEBUG_UTILS_EXTENSION_NAME] = false;
    }

    // Enable all extensions that are supported and requested.
    std::vector<vk::ExtensionProperties> instanceExtensions = vk::enumerateInstanceExtensionProperties();
    for (uint32_t i = 0; i < instanceExtensions.size(); ++i) {
        if (m_requestedInstanceExtensions.contains(instanceExtensions[i].extensionName)) {
            m_enabledInstanceExtensions.insert(instanceExtensions[i].extensionName);
        }
    }

    // Check for required extensions
    // .first is extension name, .second is required or not
    bool allReqExtsFound = true;
    for (const std::pair<const std::string, bool>& reqExt : m_requestedInstanceExtensions) {
        if (std::find(
                m_enabledInstanceExtensions.begin(),
                m_enabledInstanceExtensions.end(),
                reqExt.first.c_str()
            ) == m_enabledInstanceExtensions.end()
        ) {
            if (reqExt.second) {
                Error("Required instance extension \"%s\" not found.\n", reqExt.first.c_str());
                allReqExtsFound = false;
            } else {
                Msg("Optional instance extension \"%s\" not found.\n", reqExt.first.c_str());
            }
        }
    }

    if (!allReqExtsFound) {
        throw std::runtime_error("Can't find required instance extensions! See log.");
    }
}

std::vector<const char*> CVulkanRendererContext::FindValidationLayers() {
    if (!g_enableValidationLayers)
        return {};

    std::vector<vk::LayerProperties> availableLayers = vk::enumerateInstanceLayerProperties();
    std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    for (std::size_t i = 0; i < validationLayers.size(); ++i) {
        const char* neededLayer = validationLayers[i];

        bool layerFound = false;
        for (const auto& availableLayer : availableLayers) {
            if (!strcmp(neededLayer, availableLayer.layerName)) {
                layerFound = true;
                break;
            }
        }
        if (!layerFound) {
            validationLayers.erase(validationLayers.begin() + i);
            Warning("Can't find validation layer \"%s\"", neededLayer);
        }
    }

    return validationLayers;
}

void CVulkanRendererContext::InitializeInstance() {
    std::vector<const char*> validationLayers = FindValidationLayers();
    if (g_enableValidationLayers && validationLayers.empty()) {
        g_enableValidationLayers = false;
        Warning << "Validation layers disabled because system doesn't have vulkan validation layers!\n";
    }

    vk::ApplicationInfo appInfo {};
    appInfo.pApplicationName = "Skylabs";
    appInfo.applicationVersion = vk::makeApiVersion(0, 0, 0, 0);
    appInfo.pEngineName = "Skylabs";
    appInfo.engineVersion = vk::makeApiVersion(0, 0, 0, 0);
    appInfo.apiVersion = vk::ApiVersion13;

    vk::InstanceCreateInfo instanceInfo {};
    instanceInfo.pApplicationInfo = &appInfo;

    std::vector<const char*> enabledExtensions {};
    enabledExtensions.reserve(m_enabledInstanceExtensions.size());
    for (const std::string& extension_name : m_enabledInstanceExtensions) {
        enabledExtensions.push_back(extension_name.c_str());
    }

    instanceInfo.enabledExtensionCount = static_cast<uint32_t>(enabledExtensions.size());
    instanceInfo.ppEnabledExtensionNames = enabledExtensions.data();

    vk::DebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo {};
    if (g_enableValidationLayers) {
        debugMessengerCreateInfo.messageSeverity =
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eError |
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo;

        debugMessengerCreateInfo.messageType =
            vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
            vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
            vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
            vk::DebugUtilsMessageTypeFlagBitsEXT::eDeviceAddressBinding;

        debugMessengerCreateInfo.pfnUserCallback =
            reinterpret_cast<PFN_vkDebugUtilsMessengerCallbackEXT>(DebugCallback);

        instanceInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        instanceInfo.ppEnabledLayerNames = validationLayers.data();
        instanceInfo.pNext = &debugMessengerCreateInfo;
    } else {
        instanceInfo.enabledLayerCount = 0;
        instanceInfo.pNext = nullptr;
    }

    m_instance = vk::createInstance(instanceInfo);
    VULKAN_HPP_DEFAULT_DISPATCHER.init(m_instance);
    if (g_enableValidationLayers) {
        m_debugMessenger = m_instance.createDebugUtilsMessengerEXT(debugMessengerCreateInfo);
    }
}

vk::SurfaceKHR CVulkanRendererContext::CreateSurface() {
    vk::SurfaceKHR surface {};

    if (m_window->GetVendor() == WindowVendor::eSDL) {
        if (!SDL::Vulkan::CreateSurface(
            static_cast<SDL_Window*>(m_window->GetHandle()),
            m_instance,
            &surface
        )) {
            throw std::runtime_error(std::string("Failed to create window surface!\n") + SDL_GetError());
        }
    } else {
        assert(false && "Nothing except sdl is implemented yet."); // TODO
    }

    return surface;
}

vk::Instance CVulkanRendererContext::GetInstance() const {
    return m_instance;
}

vk::SurfaceKHR CVulkanRendererContext::GetSurface() const {
    return m_surface;
}

IWindow* CVulkanRendererContext::GetWindow() const {
    return m_window;
}

vk::DebugUtilsMessengerEXT CVulkanRendererContext::GetDebugMessenger() const {
    return m_debugMessenger;
}

VKAPI_ATTR vk::Bool32 VKAPI_CALL CVulkanRendererContext::DebugCallback(
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
