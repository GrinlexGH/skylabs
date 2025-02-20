#include "instance.hpp"

#include "console.hpp"
#include "extensions/debug_utils.hpp"

#include <sstream>

namespace {
bool HasExtension(const std::vector<const char*>& set, const std::string_view target) {
    return std::ranges::any_of(
        set,
        [&](const char* extension) { return extension == target; }
    );
}

bool HasExtension(const std::vector<vk::ExtensionProperties>& set, const std::string_view target) {
    return std::ranges::any_of(
        set,
        [&](const vk::ExtensionProperties& extension) { return extension.extensionName == target; }
    );
}

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

bool HasLayer(const std::vector<const char*>& set, const std::string_view target) {
    return std::ranges::any_of(
        set,
        [&](const char* layer) { return layer == target; }
    );
}

bool HasLayer(const std::vector<vk::LayerProperties>& set, const std::string_view target) {
    return std::ranges::any_of(
        set,
        [&](const vk::LayerProperties& layer) { return layer.layerName == target; }
    );
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
}

namespace Vulkan {
CInstance::CInstance(
    const Config& config,
    const std::unordered_map<const char*, bool>& extensions,
    const std::vector<const char*>& layers
) {
    VULKAN_HPP_DEFAULT_DISPATCHER.init();

    std::uint32_t apiVersion = vk::ApiVersion10;

    if (VULKAN_HPP_DEFAULT_DISPATCHER.vkEnumerateInstanceVersion) {
        apiVersion = std::max(vk::enumerateInstanceVersion(), apiVersion);
    }

    Msg("Vulkan version: {}.{}.{}.{}",
        vk::apiVersionVariant(apiVersion),
        vk::apiVersionMajor(apiVersion),
        vk::apiVersionMinor(apiVersion),
        vk::apiVersionPatch(apiVersion)
    );

    //====================
    vk::ApplicationInfo appInfo {};
    appInfo.pApplicationName = config.gameName;
    appInfo.applicationVersion = config.m_gameVersion;
    appInfo.pEngineName = config.engineName;
    appInfo.engineVersion = config.m_engineBuild;
    appInfo.apiVersion = apiVersion;

    //====================
    std::vector<const char*> enabledExtensions {};

#if _DEBUG
    const bool isDebugUtilsAvailable = EnableExtension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME, enabledExtensions);
#endif

    // if required extension is missing, generate error message
    std::vector<const char*> missingExtensions {};
    for (const auto& [name, required] : extensions) {
        if (const bool isAvailable = EnableExtension(name, enabledExtensions); required && !isAvailable) {
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
#if _DEBUG
    EnableLayer("VK_LAYER_KHRONOS_validation", enabledLayers);
#endif

    for (const auto& name : layers) {
        EnableLayer(name, enabledLayers);
    }

    //====================
    void* pNext = nullptr;

#if _DEBUG
    if (isDebugUtilsAvailable) {
        AppendToPNextChain(pNext, &CDebugUtils::CreateInfo());
    }
#endif

    //====================
    vk::InstanceCreateInfo instanceCreateInfo;
    instanceCreateInfo.pApplicationInfo = &appInfo;
    instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(enabledExtensions.size());
    instanceCreateInfo.ppEnabledExtensionNames = enabledExtensions.data();
    instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(enabledLayers.size());
    instanceCreateInfo.ppEnabledLayerNames = enabledLayers.data();
    instanceCreateInfo.pNext = pNext;

    m_handle = createInstance(instanceCreateInfo);
    VULKAN_HPP_DEFAULT_DISPATCHER.init(m_handle);

    //====================
#if _DEBUG
    if (isDebugUtilsAvailable) {
        m_debugUtils = std::make_unique<CDebugUtils>(m_handle);
    }
#endif
}

CInstance::~CInstance() {
    if (!m_handle) {
        return;
    }

    m_handle.destroy();
}
}
