#include "instance.hpp"

#include "console.hpp"
#include "extensions/extensions.hpp"
#include "extensions/debug_utils.hpp"

#include <sstream>

namespace {
bool EnableExtension(const char* name, std::vector<const char*>& enabledExtensions) {
    static std::vector<vk::ExtensionProperties> availableExtensions = vk::enumerateInstanceExtensionProperties();

    if (HasExtension(availableExtensions, name)) {
        if (!HasExtension(enabledExtensions, name)) {
            enabledExtensions.push_back(name);
        }
    } else {
        return false;
    }

    return true;
}

bool EnableLayer(const char* name, std::vector<const char*>& enabledLayers) {
    static std::vector<vk::LayerProperties> availableLayers = vk::enumerateInstanceLayerProperties();

    if (HasLayer(availableLayers, name)) {
        if (!HasLayer(enabledLayers, name)) {
            enabledLayers.push_back(name);
        }
    }
    else {
        return false;
    }

    return true;
}
}

namespace Vulkan {
CInstance::CInstance(
    const Config& config,
    const std::unordered_map<const char*, bool>& extensions,
    const std::vector<const char*>& layers
) {
    VULKAN_HPP_DEFAULT_DISPATCHER.init();

    m_apiVersion = vk::ApiVersion10;

    if (VULKAN_HPP_DEFAULT_DISPATCHER.vkEnumerateInstanceVersion) {
        m_apiVersion = std::max(vk::enumerateInstanceVersion(), m_apiVersion);
    }

    Msg("Vulkan version: {}.{}.{}.{}",
        vk::apiVersionVariant(m_apiVersion),
        vk::apiVersionMajor(m_apiVersion),
        vk::apiVersionMinor(m_apiVersion),
        vk::apiVersionPatch(m_apiVersion)
    );

    //====================
    vk::ApplicationInfo appInfo {};
    appInfo.pApplicationName = config.m_gameName;
    appInfo.applicationVersion = config.m_gameVersion;
    appInfo.pEngineName = config.m_engineName;
    appInfo.engineVersion = config.m_engineBuild;
    appInfo.apiVersion = m_apiVersion;

    //====================
#ifdef DEBUG
    const bool isDebugUtilsAvailable = EnableExtension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME, m_enabledExtensions);
#endif

    // if required extension is missing, put it into error message
    std::vector<const char*> missingExtensions {};
    for (const auto& [name, required] : extensions) {
        if (const bool isAvailable = EnableExtension(name, m_enabledExtensions); required && !isAvailable) {
            missingExtensions.push_back(name);
        }
    }

    if (!missingExtensions.empty()) {
        std::ostringstream error;
        error << "System doesn't have required instance extensions:\n";
        for (const auto name : missingExtensions) {
            error << name << '\n';
        }
        throw std::runtime_error(error.str());
    }

    //====================
    std::vector<const char*> enabledLayers;
#ifdef DEBUG
    EnableLayer("VK_LAYER_KHRONOS_validation", enabledLayers);
#endif

    for (const auto& name : layers) {
        EnableLayer(name, enabledLayers);
    }

    //====================
    void* pNext = nullptr;

#ifdef DEBUG
    if (isDebugUtilsAvailable) {
        AppendToPNextChain(pNext, &CDebugUtils::CreateInfo());
    }
#endif

    //====================
    vk::InstanceCreateInfo instanceCreateInfo;
    instanceCreateInfo.pApplicationInfo = &appInfo;
    instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(m_enabledExtensions.size());
    instanceCreateInfo.ppEnabledExtensionNames = m_enabledExtensions.data();
    instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(enabledLayers.size());
    instanceCreateInfo.ppEnabledLayerNames = enabledLayers.data();
    instanceCreateInfo.pNext = pNext;

    m_handle = createInstance(instanceCreateInfo);
    VULKAN_HPP_DEFAULT_DISPATCHER.init(m_handle);

    //====================
#ifdef DEBUG
    if (isDebugUtilsAvailable) {
        m_debugUtils = std::make_unique<CDebugUtils>(m_handle);
    }
#endif
}

CInstance::~CInstance() {
    if (!m_handle) {
        return;
    }

    m_debugUtils->Destroy(m_handle);

    m_handle.destroy();
}
}
