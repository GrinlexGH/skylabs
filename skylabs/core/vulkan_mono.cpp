#include <skylabs/core/vulkan_mono.hpp>
#include <skylabs/public/logging.hpp>
#include <skylabs/core/SDL/context.hpp>
#include <skylabs/core/SDL/vulkan/window.hpp>

#include <vulkan/vulkan_raii.hpp>

#include "project_info.hpp"

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

namespace Vulkan {
void CVulkanMono::Run() {
    CTimer timer;

    const SDL::CContext sdl(SDL_INIT_VIDEO);

    const SDL::Vulkan::CWindow window("Skylabs", 640, 480, SDL_WINDOW_RESIZABLE);
    SDL_SetWindowRelativeMouseMode(window.Handle(), true);

    VULKAN_HPP_DEFAULT_DISPATCHER.init();
    vk::raii::Context context;
    if (context.getDispatcher()->vkEnumerateInstanceVersion) {
        std::uint32_t apiVersion = context.enumerateInstanceVersion();
        Log::Debug(
            "Available Vulkan version: {}.{}.{}, but anyways I will use Vulkan 1.0",
            vk::apiVersionMajor(apiVersion),
            vk::apiVersionMinor(apiVersion),
            vk::apiVersionPatch(apiVersion)
        );
    }

    std::vector<vk::LayerProperties> availableLayers = context.enumerateInstanceLayerProperties();

    for (const auto& layerProperties : availableLayers) {
        std::string_view layerName = layerProperties.layerName;
        Log::Debug("Layer {}", layerName);
    }

    std::vector<vk::ExtensionProperties> availableValidationLayerExtensions = context.enumerateInstanceExtensionProperties(std::string { "VK_LAYER_KHRONOS_validation" });
    Log::Debug("VK_LAYER_KHRONOS_validation extensions:");
    for (const auto& extensionProperties : availableValidationLayerExtensions) {
        Log::Debug("    Extension {}", std::string_view { extensionProperties.extensionName });
    }

    std::vector<vk::ExtensionProperties> availableExtensions = context.enumerateInstanceExtensionProperties();
    for (const auto& extensionProperties : availableExtensions) {
        Log::Debug("Extension {}", std::string_view { extensionProperties.extensionName });
    }

    vk::ApplicationInfo applicationInfo {};
    applicationInfo.pNext = nullptr;
    applicationInfo.pApplicationName = Skylabs::GAME_NAME;
    applicationInfo.applicationVersion = vk::makeApiVersion(0, Skylabs::VERSION_MAJOR, Skylabs::VERSION_MINOR, Skylabs::VERSION_PATCH);
    applicationInfo.pEngineName = Skylabs::NAME;
    applicationInfo.engineVersion = vk::makeApiVersion(0, Skylabs::VERSION_MAJOR, Skylabs::VERSION_MINOR, Skylabs::VERSION_PATCH);
    applicationInfo.apiVersion = vk::ApiVersion10;

    std::array<const char* const, 0> enabledLayers = {  };
    std::array<const char* const, 0> enabledExtensions = {  };

    vk::InstanceCreateInfo instanceCreateInfo {};
    instanceCreateInfo.pNext = nullptr;
    instanceCreateInfo.flags = {};
    instanceCreateInfo.pApplicationInfo = &applicationInfo;
    instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(enabledLayers.size());
    instanceCreateInfo.ppEnabledLayerNames = !enabledLayers.empty() ? enabledLayers.data() : nullptr;
    instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(enabledExtensions.size());
    instanceCreateInfo.ppEnabledExtensionNames = !enabledExtensions.empty() ? enabledExtensions.data() : nullptr;

    vk::raii::Instance instance = context.createInstance(instanceCreateInfo);
}
}
