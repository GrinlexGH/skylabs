#include "instance.hpp"

#include "physical_device.hpp"
#include "project_info.hpp"

#include <sstream>

namespace {
#ifdef DEBUG
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
#endif
}

namespace Vulkan {
CInstance::CInstance(
    const std::unordered_map<const char*, bool>& extensions,
    const std::vector<const char*>& layers
) {
    if (m_context.getDispatcher()->vkEnumerateInstanceVersion) {
        m_apiVersion = m_context.enumerateInstanceVersion();
    }

    Log::Info(
        "Vulkan version: {}.{}.{}.{}",
        vk::apiVersionVariant(m_apiVersion),
        vk::apiVersionMajor(m_apiVersion),
        vk::apiVersionMinor(m_apiVersion),
        vk::apiVersionPatch(m_apiVersion)
    );

    //====================
    vk::ApplicationInfo appInfo;
    appInfo.pApplicationName = Skylabs::GAME_NAME;
    appInfo.applicationVersion = vk::makeApiVersion(0, Skylabs::VERSION_MAJOR, Skylabs::VERSION_MINOR, Skylabs::VERSION_PATCH);
    appInfo.pEngineName = Skylabs::NAME;
    appInfo.engineVersion = vk::makeApiVersion(0, Skylabs::VERSION_MAJOR, Skylabs::VERSION_MINOR, Skylabs::VERSION_PATCH);
    appInfo.apiVersion = m_apiVersion;

    //====================
#ifdef DEBUG
    bool isDebugUtilsAvailable = EnableExtension(vk::EXTDebugUtilsExtensionName);
    std::vector<vk::LayerProperties> layerProperties = m_context.enumerateInstanceLayerProperties();
    if (!isDebugUtilsAvailable) {
        for (const vk::LayerProperties& layer: layerProperties) {
            auto layer_exts = m_context.enumerateInstanceExtensionProperties({layer.layerName});
            isDebugUtilsAvailable = layer_exts.begin() != std::find_if(
              layer_exts.begin(), layer_exts.end(),[](vk::ExtensionProperties extensionProperties) {
                return strcmp(extensionProperties.extensionName,
                vk::EXTDebugUtilsExtensionName) == 0;
              });
            if (isDebugUtilsAvailable) {
                break;
            }
        }
    }
#endif

    // if required extension is missing, put it into error message
    std::vector<const char*> missingExtensions;
    missingExtensions.reserve(extensions.size());
    m_enabledExtensions.reserve(extensions.size());

    for (const auto& [name, required] : extensions) {
        if (!EnableExtension(name) && required) {
            missingExtensions.push_back(name);
        }
    }

    if (!missingExtensions.empty()) {
        std::ostringstream error;
        error << "System doesn't have required instance extensions:\n";
        for (const auto name : missingExtensions) {
            error << '\t' << name << '\n';
        }
        throw std::runtime_error(error.str());
    }

    //====================
    std::vector<const char*> enabledLayers;
    enabledLayers.reserve(layers.size());
    std::vector<const char*> missingLayers;
    missingLayers.reserve(layers.size());

#ifdef DEBUG
    if (!EnableLayer("VK_LAYER_KHRONOS_validation", enabledLayers)) {
        missingLayers.push_back("VK_LAYER_KHRONOS_validation");
    }
#endif

    for (const auto& name : layers) {
        if (!EnableLayer(name, enabledLayers)) {
            missingLayers.push_back(name);
        }
    }

    if (!missingLayers.empty()) {
        std::ostringstream error;
        error << "System doesn't have vulkan layers:\n";
        for (const char* const name : missingLayers) {
            error << '\t' << name << '\n';
        }
        Log::Debug("{}", error.str());
    }

    //====================
    void* pNext = nullptr;

#ifdef DEBUG
    vk::DebugUtilsMessengerCreateInfoEXT debugUtilsCreateInfo {
        { },

        vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,

        vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
        vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
        vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,

        DebugCallback
    };

    if (isDebugUtilsAvailable) {
        AppendToPNextChain(pNext, &debugUtilsCreateInfo);
    }
#endif

    //====================
    vk::InstanceCreateInfo createInfo;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = static_cast<std::uint32_t>(m_enabledExtensions.size());
    createInfo.ppEnabledExtensionNames = m_enabledExtensions.data();
    createInfo.enabledLayerCount = static_cast<std::uint32_t>(enabledLayers.size());
    createInfo.ppEnabledLayerNames = enabledLayers.data();
    createInfo.pNext = pNext;

    m_handle = m_context.createInstance(createInfo);

    //====================
#ifdef DEBUG
    if (isDebugUtilsAvailable) {
        m_debugUtilsMessenger = m_handle.createDebugUtilsMessengerEXT(debugUtilsCreateInfo);
    }
#endif

    QueryPhysicalDevices();
}

auto CInstance::EnableExtension(const char* name) -> bool {
    static const std::vector<vk::ExtensionProperties>& availableExtensions = m_context.enumerateInstanceExtensionProperties();

    if (HasExtension(availableExtensions, name)) {
        if (!HasExtension(m_enabledExtensions, name)) {
            m_enabledExtensions.emplace_back(name);
        }
    } else {
        return false;
    }

    return true;
}

auto CInstance::EnableLayer(const char* name, std::vector<const char*>& enabledLayers) const -> bool {
    const static std::vector<vk::LayerProperties> availableLayers = m_context.enumerateInstanceLayerProperties();

    if (HasLayer(availableLayers, name)) {
        if (!HasLayer(enabledLayers, name)) {
            enabledLayers.emplace_back(name);
        }
    } else {
        return false;
    }

    return true;
}

auto CInstance::QueryPhysicalDevices() -> void {
    vk::raii::PhysicalDevices physicalDevices { m_handle };
    if (physicalDevices.empty()) {
        throw std::runtime_error("Couldn't find a physical device that supports Vulkan!");
    }

    for (vk::raii::PhysicalDevice& physicalDevice : physicalDevices) {
        m_physicalDevices.push_back(std::make_unique<CPhysicalDevice>(std::move(physicalDevice)));
    }
}
}
