#include "instance.hpp"

#include "console.hpp"
#include "extensions/debug_messenger.hpp"
#include "extensions/extension_manager.hpp"

namespace Vulkan
{
void CInstance::Create(IVulkanWindow* window) {
    VULKAN_HPP_DEFAULT_DISPATCHER.init();

    uint32_t instanceVersion = vk::ApiVersion10;
    if (VULKAN_HPP_DEFAULT_DISPATCHER.vkEnumerateInstanceVersion) {
        instanceVersion = vk::enumerateInstanceVersion();
    }
    Msg(
        "Vulkan version: {}.{}.{}.{}",
        vk::apiVersionVariant(instanceVersion),
        vk::apiVersionMajor(instanceVersion),
        vk::apiVersionMinor(instanceVersion),
        vk::apiVersionPatch(instanceVersion)
    );

    vk::ApplicationInfo applicationInfo;
    applicationInfo.pApplicationName = "Hotline miami 4";
    applicationInfo.applicationVersion = vk::makeApiVersion(0, 0, 0, 0);
    applicationInfo.pEngineName = "Skylabs";
    applicationInfo.engineVersion = vk::makeApiVersion(0, 0, 0, 0);
    applicationInfo.apiVersion = instanceVersion;

    //====================
    const std::vector<vk::ExtensionProperties> availableExtensions = vk::enumerateInstanceExtensionProperties();

    std::vector<const char*> enabledExtensions;
    std::vector<const char*> missingExtensions;

    for (auto extension : window->GetRequiredInstanceExtensions()) {
        bool isExtFound = false;

        if (HasExtension(availableExtensions, extension)) {
            isExtFound = true;
            enabledExtensions.push_back(extension);
        }

        if (!isExtFound) {
            missingExtensions.push_back(extension);
        }
    }

    if (!missingExtensions.empty()) {
        std::string message = "Required vulkan extensions not found:\n";
        for (const auto ext : missingExtensions) {
            message.append(ext);
            message.append("\n");
        }
        throw std::runtime_error(message);
    }

    //====================
    std::vector<const char*> enabledLayers;

#ifdef _DEBUG
    vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfo;

    const bool debugMessengerAvailable = PrepareDebugUtilsExtension(
        vk::enumerateInstanceLayerProperties(),
        availableExtensions,
        enabledLayers,
        enabledExtensions,
        debugUtilsMessengerCreateInfo
    );
#endif

    //====================
    const void* pNextChain = nullptr;

#ifdef _DEBUG
    if (debugMessengerAvailable) {
        pNextChain = &debugUtilsMessengerCreateInfo;
    }
#endif

    //====================
    vk::InstanceCreateInfo instanceCreateInfo;
    instanceCreateInfo.pApplicationInfo = &applicationInfo;
    instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(enabledExtensions.size());
    instanceCreateInfo.ppEnabledExtensionNames = enabledExtensions.data();
    instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(enabledLayers.size());
    instanceCreateInfo.ppEnabledLayerNames = enabledLayers.data();
    instanceCreateInfo.pNext = pNextChain;

    m_handle = createInstance(instanceCreateInfo);
    VULKAN_HPP_DEFAULT_DISPATCHER.init(m_handle);

#ifdef _DEBUG
    if (debugMessengerAvailable) {
        m_debugMessenger = m_handle.createDebugUtilsMessengerEXT(debugUtilsMessengerCreateInfo);
    }
#endif
}

CInstance::~CInstance() {
    if (!m_handle) {
        return;
    }

    if (m_debugMessenger) {
        m_handle.destroyDebugUtilsMessengerEXT(m_debugMessenger);
    }

    m_handle.destroy();
}
}
