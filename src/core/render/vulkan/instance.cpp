#include "instance.hpp"

#include "console.hpp"
#include "extensions/debug_messenger.hpp"
#include "extensions/extension_manager.hpp"

namespace
{
void SetupRequiredExtensions(
    const std::vector<vk::ExtensionProperties>& availableExtensions,
    const std::vector<const char*>& requiredExtensions,
    std::vector<const char*>& enabledExtensions
) {
    enabledExtensions.reserve(requiredExtensions.size());

    std::vector<const char*> missingExtensions;

    for (auto extension : requiredExtensions) {
        if (HasExtension(availableExtensions, extension)) {
            enabledExtensions.push_back(extension);
        } else {
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
}

void SetupExtensions(
    const std::vector<vk::ExtensionProperties>& availableExtensions,
    const std::vector<const char*>& requiredExtensions,
    std::vector<const char*>& enabledExtensions
) {
    SetupRequiredExtensions(
        availableExtensions,
        requiredExtensions,
        enabledExtensions
    );
}

void SetupLayers(
    const std::vector<vk::LayerProperties>& availableLayers,
    const std::vector<vk::ExtensionProperties>& availableExtensions,
    CInstanceExtensionsCreateInfo& createInfos,
    std::vector<const char*>& enabledLayers,
    std::vector<const char*>& enabledExtensions,
    void*& pNextChain
) {
    AddDebugUtilsLayer(
        availableLayers,
        availableExtensions,
        createInfos,
        enabledLayers,
        enabledExtensions,
        pNextChain
    );
}
}

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
    std::vector<const char*> enabledExtensions;
    std::vector<const char*> enabledLayers;
    void* pNextChain = nullptr;

    CInstanceExtensionsCreateInfo createInfos;

    const std::vector<vk::ExtensionProperties> availableExtensions = vk::enumerateInstanceExtensionProperties();

    SetupExtensions(
        availableExtensions,
        window->GetRequiredInstanceExtensions(),
        enabledExtensions
    );

    SetupLayers(
        vk::enumerateInstanceLayerProperties(),
        availableExtensions,
        createInfos,
        enabledLayers,
        enabledExtensions,
        pNextChain
    );

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
    if (HasExtension(enabledExtensions, VK_EXT_DEBUG_UTILS_EXTENSION_NAME)) {
        m_debugMessenger = m_handle.createDebugUtilsMessengerEXT(createInfos.m_debugUtilsMessengerCreateInfo);
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
