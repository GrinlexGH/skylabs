#include <skylabs/core/render/vulkan/context/instance.hpp>
#include <skylabs/core/render/vulkan/context/physical_device.hpp>
#include "project_info.hpp"

#include <unordered_set>
#include <ranges>

namespace std {
    template<>
    struct hash<vk::LayerProperties> {
        size_t operator()(vk::LayerProperties const& layer) const {
            return hash<string_view>{}(layer.layerName);
        }
    };
}

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
CInstance::CInstance(std::nullptr_t) {}

CInstance::CInstance(
    const std::unordered_map<std::string_view, bool>& extensions,
    const std::vector<std::string_view>& layers
) {
    VULKAN_HPP_DEFAULT_DISPATCHER.init();

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
    std::vector<const char*> enabledLayers = EnableLayers(layers);

    //====================
    std::vector<const char*> enabledExtensions = EnableExtensions(extensions);

    //====================
    void* pNext = nullptr;

#ifdef DEBUG
    const bool isDebugUtilsAvailable = IsExtensionEnabled(vk::EXTDebugUtilsExtensionName);

    vk::DebugUtilsMessengerCreateInfoEXT debugUtilsCreateInfo {
        {},

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
    createInfo.enabledExtensionCount = static_cast<uint32_t>(m_enabledExtensions.size());
    createInfo.ppEnabledExtensionNames = enabledExtensions.data();
    createInfo.enabledLayerCount = static_cast<uint32_t>(m_enabledLayers.size());
    createInfo.ppEnabledLayerNames = enabledLayers.data();
    createInfo.pNext = pNext;

    m_handle = vk::raii::Instance { m_context, createInfo };
    VULKAN_HPP_DEFAULT_DISPATCHER.init(*m_handle);

    //====================
#ifdef DEBUG
    if (isDebugUtilsAvailable) {
        m_debugUtilsMessenger = vk::raii::DebugUtilsMessengerEXT { m_handle, debugUtilsCreateInfo };
    }
#endif

    QueryPhysicalDevices();
}

auto CInstance::GetAvailableLayers() const -> std::unordered_set<std::string_view> {
    static const std::vector<vk::LayerProperties> layers = m_context.enumerateInstanceLayerProperties();
    static const std::unordered_set<std::string_view> layerMap =
        layers |
        // Transform to pick only layer name
        std::views::transform([](const vk::LayerProperties& layer) {
            return std::string_view { layer.layerName };
        }) |
        std::ranges::to<std::unordered_set<std::string_view>>();

    return layerMap;
}

auto CInstance::GetAvailableExtensions() const -> std::unordered_set<std::string_view> {
    static const std::vector<vk::ExtensionProperties>& globalExtensions = m_context.enumerateInstanceExtensionProperties();
    static const std::unordered_set<std::string_view> globalExtensionsMap =
        globalExtensions |
        // Transform to pick only extension name
        std::views::transform([](const vk::ExtensionProperties& layer) {
            return std::string_view { layer.extensionName };
        }) |
        std::ranges::to<std::unordered_set<std::string_view>>();

    return globalExtensionsMap;
}

auto CInstance::EnableLayers(const std::vector<std::string_view>& layers) -> std::vector<const char*> {
    const std::unordered_set<std::string_view> availableLayers = GetAvailableLayers();
    std::unordered_set<std::string_view> pendingLayers { layers.begin(), layers.end() };

    // Currently (27.08.2025) validation layer cause sigsegv with gdb on windows, so don't use gdb on windows
    // (06.12.2025) Fixed?
#ifdef DEBUG
    pendingLayers.insert("VK_LAYER_KHRONOS_validation");
#endif

    std::vector<const char*> enabledLayers;
    enabledLayers.reserve(pendingLayers.size());

    m_enabledLayers.reserve(pendingLayers.size());

    for (const std::string_view& layerName : pendingLayers) {
        if (availableLayers.contains(layerName)) {
            enabledLayers.push_back(layerName.data());
            m_enabledLayers.insert(layerName);
        } else {
            Log::Debug("System doesn't have vulkan layer \"{}\"", layerName);
        }
    }

    return enabledLayers;
}

auto CInstance::EnableExtensions(const std::unordered_map<std::string_view, bool>& extensions) -> std::vector<const char*> {
    const std::unordered_set<std::string_view> availableExtensions = GetAvailableExtensions();

    std::vector<const char*> enabledExtensions;
    enabledExtensions.reserve(extensions.size());

    std::vector<std::pair<std::string_view, bool>> missingExtensions;
    missingExtensions.reserve(extensions.size());

    m_enabledExtensions.reserve(extensions.size() + 1);

    for (const auto& [extensionName, required] : extensions) {
        if (!availableExtensions.contains(extensionName)) {
            missingExtensions.emplace_back(extensionName, required);
        } else {
            enabledExtensions.push_back(extensionName.data());
            m_enabledExtensions.insert(extensionName);
        }
    }

#ifdef DEBUG
    if (availableExtensions.contains(vk::EXTDebugUtilsExtensionName)) {
        enabledExtensions.push_back(vk::EXTDebugUtilsExtensionName);
        m_enabledExtensions.insert(vk::EXTDebugUtilsExtensionName);
    } else {
        missingExtensions.emplace_back(vk::EXTDebugUtilsExtensionName, false);
    }
#endif

    if (!missingExtensions.empty()) {
        for (const std::string_view& layerName : m_enabledLayers) {
            const std::vector<vk::ExtensionProperties> availableExtensions =
                m_context.enumerateInstanceExtensionProperties(std::string { layerName });

            std::erase_if(missingExtensions, [&](const std::pair<std::string_view, bool> extension) {
                const auto [extensionName, required] = extension;
                if (HasExtension(availableExtensions, extensionName)) {
                    enabledExtensions.push_back(extensionName.data());
                    m_enabledExtensions.insert(extensionName);
                    return true;
                }
                if (!required)
                    return true;
                return false;
            });
        }
    }

    if (!missingExtensions.empty()) {
        std::string error;
        error.reserve((missingExtensions.size() * 20) + 50);
        error += "System doesn't have required instance extensions:\n";
        for (const auto [name, _] : missingExtensions) {
            error += '\t';
            error += name;
            error += '\n';
        }
        throw std::runtime_error { error };
    }

    return enabledExtensions;
}

auto CInstance::QueryPhysicalDevices() -> void {
    vk::raii::PhysicalDevices physicalDevices { m_handle };
    if (physicalDevices.empty()) {
        throw std::runtime_error { "Couldn't find a physical device that supports Vulkan!" };
    }

    m_physicalDevices.reserve(physicalDevices.size());
    for (vk::raii::PhysicalDevice& physicalDevice : physicalDevices) {
        m_physicalDevices.emplace_back(std::move(physicalDevice));
    }
}
}
