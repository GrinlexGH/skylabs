#include <skylabs/core/vulkan_mono.hpp>

#include <skylabs/core/SDL/context.hpp>
#include <skylabs/core/SDL/vulkan/window.hpp>
#include <skylabs/public/logging.hpp>
#include "project_info.hpp"

#include <vulkan/vulkan_raii.hpp>
#include <fmt/ranges.h>

#include <unordered_set>

// Fix defines
#ifdef CreateWindow
#undef CreateWindow
#endif

class CTimer
{
public:
    CTimer() {
        Log::Debug("Timer start");
        m_start = std::chrono::steady_clock::now();
    }

    ~CTimer() {
        const std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
        Log::Debug("{} ms", std::chrono::duration_cast<std::chrono::milliseconds>(end - m_start).count());
    }

private:
    std::chrono::steady_clock::time_point m_start;
};

bool HasLayer(const std::vector<vk::LayerProperties>& set, const std::string_view target) {
    return std::ranges::any_of(
        set, [&](const vk::LayerProperties& layer) { return layer.layerName == target; }
    );
}

void AppendToPNextChain(void*& currentChain, void* newExtension) {
    if (currentChain == nullptr) {
        currentChain = newExtension;
        return;
    }

    auto* current = static_cast<vk::BaseOutStructure*>(currentChain);
    while (current->pNext != nullptr) {
        current = current->pNext;
    }

    current->pNext = static_cast<vk::BaseOutStructure*>(newExtension);
}

VKAPI_ATTR vk::Bool32 VKAPI_CALL DebugCallback(
    vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    vk::DebugUtilsMessageTypeFlagsEXT /*messageTypes*/,
    const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* /*pUserData*/
) {
    switch (messageSeverity) {
        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose:
        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo:
            Log::Info("{}", pCallbackData->pMessage);
            break;
        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning:
            Log::Warning("{}", pCallbackData->pMessage);
            break;
        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eError:
            Log::Error("{}", pCallbackData->pMessage);
            break;
    }

    return vk::False;
}

namespace Vulkan {
void CVulkanMono::CreateWindow() {
    m_SDLContext = SDL::CContext { SDL_INIT_VIDEO };
    m_SDLWindow = SDL::Vulkan::CWindow { "Skylabs", 640, 480, SDL_WINDOW_RESIZABLE };
    SDL_SetWindowRelativeMouseMode(*m_SDLWindow, true);
}

void CVulkanMono::Run() {
    CreateWindow();

    CTimer timer;

    VULKAN_HPP_DEFAULT_DISPATCHER.init();
    vk::raii::Context context;
    if (context.getDispatcher()->vkEnumerateInstanceVersion) {
        const std::uint32_t apiVersion = context.enumerateInstanceVersion();
        Log::Debug(
            "Available Vulkan version: {}.{}.{}, but anyways I will use Vulkan 1.0",
            vk::apiVersionMajor(apiVersion),
            vk::apiVersionMinor(apiVersion),
            vk::apiVersionPatch(apiVersion)
        );
    }

    // Collect all available global extensions
    std::vector<std::string> availableExtensions;
    {
        const std::vector<vk::ExtensionProperties> globalExtensions = context.enumerateInstanceExtensionProperties();
        availableExtensions.reserve(globalExtensions.size());
        for(const auto& p : globalExtensions) {
            availableExtensions.emplace_back(p.extensionName.data());
        }
    }

    // Enable validation layer
    std::vector<const char*> enabledLayers { };
    {
        const std::vector<vk::LayerProperties> availableLayers = context.enumerateInstanceLayerProperties();

        if (constexpr auto validationLayerName = "VK_LAYER_KHRONOS_validation";
            HasLayer(availableLayers, validationLayerName)
        ) {
            Log::Debug("Enabling {}...", validationLayerName);
            enabledLayers.push_back(validationLayerName);

            // If VK_LAYER_KHRONOS_validation is enabled, extensions from it are also available
            for(const auto& p : context.enumerateInstanceExtensionProperties(std::string { validationLayerName })) {
                availableExtensions.emplace_back(p.extensionName.data());
            }
        }
    }

    // Sort for binary searching
    std::ranges::sort(availableExtensions);

    // Enable extensions
    std::vector<std::string> activeExtensions { };
    void* pNext = nullptr;

    for (const auto ext : m_SDLWindow.GetRequiredInstanceExtensions()) {
        if (std::ranges::binary_search(availableExtensions, ext))
            activeExtensions.push_back(ext);
    }

    bool isDebugUtilsAvailable = false;
    vk::DebugUtilsMessengerCreateInfoEXT debugUtilsCreateInfo {};
    if (std::ranges::binary_search(availableExtensions, vk::EXTDebugUtilsExtensionName)) {
        isDebugUtilsAvailable = true;

        activeExtensions.push_back(vk::EXTDebugUtilsExtensionName);
        debugUtilsCreateInfo.messageSeverity =
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose
            | vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo
            | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning
            | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;
        debugUtilsCreateInfo.messageType =
            vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral
            | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation
            | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;
        debugUtilsCreateInfo.pfnUserCallback = DebugCallback;

        AppendToPNextChain(pNext, &debugUtilsCreateInfo);
    }

    // Sort for binary searching
    std::ranges::sort(activeExtensions);
    activeExtensions.erase(std::ranges::unique(activeExtensions).begin(), activeExtensions.end());

    // For instance creation
    std::vector<const char*> enabledExtensions { };
    enabledExtensions.reserve(activeExtensions.size());
    for (const auto& ext : activeExtensions) {
        enabledExtensions.push_back(ext.c_str());
    }

    vk::ApplicationInfo applicationInfo {};
    applicationInfo.pNext = nullptr;
    applicationInfo.pApplicationName = Skylabs::GAME_NAME;
    applicationInfo.applicationVersion = vk::makeApiVersion(0, Skylabs::VERSION_MAJOR, Skylabs::VERSION_MINOR, Skylabs::VERSION_PATCH);
    applicationInfo.pEngineName = Skylabs::NAME;
    applicationInfo.engineVersion = vk::makeApiVersion(0, Skylabs::VERSION_MAJOR, Skylabs::VERSION_MINOR, Skylabs::VERSION_PATCH);
    applicationInfo.apiVersion = vk::ApiVersion10;

    vk::InstanceCreateInfo instanceCreateInfo {};
    instanceCreateInfo.pNext = nullptr;
    instanceCreateInfo.flags = {};
    instanceCreateInfo.pApplicationInfo = &applicationInfo;
    instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(enabledLayers.size());
    instanceCreateInfo.ppEnabledLayerNames = !enabledLayers.empty() ? enabledLayers.data() : nullptr;
    instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(enabledExtensions.size());
    instanceCreateInfo.ppEnabledExtensionNames = !enabledExtensions.empty() ? enabledExtensions.data() : nullptr;

    vk::raii::Instance instance { context, instanceCreateInfo };
    VULKAN_HPP_DEFAULT_DISPATCHER.init(*instance);

    vk::raii::DebugUtilsMessengerEXT debugUtilsMessenger { nullptr };
    if (isDebugUtilsAvailable) {
        debugUtilsMessenger = vk::raii::DebugUtilsMessengerEXT { instance, debugUtilsCreateInfo };
    }
}
}
