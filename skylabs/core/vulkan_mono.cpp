#include <skylabs/core/vulkan_mono.hpp>
#include <skylabs/public/logging.hpp>
#include <skylabs/core/SDL/context.hpp>
#include <skylabs/core/SDL/vulkan/window.hpp>
#include "project_info.hpp"

#include <vulkan/vulkan_raii.hpp>

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

inline bool HasLayer(const std::vector<vk::LayerProperties>& set, const std::string_view target) {
    return std::ranges::any_of(
        set, [&](const vk::LayerProperties& layer) { return layer.layerName == target; }
    );
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

    const std::vector<vk::LayerProperties> availableLayers = context.enumerateInstanceLayerProperties();
    std::vector<vk::ExtensionProperties> availableExtensions = context.enumerateInstanceExtensionProperties();

    std::vector<const char*> enabledLayers { };
    std::array<const char*, 0> enabledExtensions = {  };

    if (HasLayer(availableLayers, "VK_LAYER_KHRONOS_validation")) {
        Log::Debug("Enabling VK_LAYER_KHRONOS_validation...");
        enabledLayers.push_back("VK_LAYER_KHRONOS_validation");

        // If VK_LAYER_KHRONOS_validation is enabled, extensions from it are also available
        for (const auto& extensionProperties : context.enumerateInstanceExtensionProperties(std::string { "VK_LAYER_KHRONOS_validation" })) {
            Log::Debug("Adding extension from VK_LAYER_KHRONOS_validation: {}", std::string_view { extensionProperties.extensionName });
            availableExtensions.push_back(extensionProperties.extensionName);
        }
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

    vk::raii::Instance instance = context.createInstance(instanceCreateInfo);
    VULKAN_HPP_DEFAULT_DISPATCHER.init(*instance);
}
}
