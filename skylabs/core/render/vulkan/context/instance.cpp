#include <skylabs/core/render/vulkan/context/instance.hpp>
#include <skylabs/core/render/vulkan/context/physical_device.hpp>
#include "project_info.hpp"

#include <ranges>

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
    const std::span<RequestedExtension> extensions,
    const std::span<std::string_view> layers
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
    const std::vector<const char*> enabledLayers = EnableLayers(layers);

    //====================
    const std::vector<const char*> enabledExtensions = EnableExtensions(extensions);

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
    createInfo.enabledExtensionCount = static_cast<uint32_t>(enabledExtensions.size());
    createInfo.ppEnabledExtensionNames = enabledExtensions.data();
    createInfo.enabledLayerCount = static_cast<uint32_t>(enabledLayers.size());
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

std::vector<const char*> CInstance::EnableLayers(const std::span<std::string_view> requestedLayers) {
    static const std::vector<vk::LayerProperties> layerProps = m_context.enumerateInstanceLayerProperties();
    static const std::unordered_set<std::string_view> availableLayerNames =
        layerProps
        | std::views::transform([&](const vk::LayerProperties& layer) -> std::string_view { return layer.layerName; })
        | std::ranges::to<std::unordered_set>();

    std::vector<const char*> enabledLayers;
    enabledLayers.reserve(requestedLayers.size() + 1);

    m_enabledLayers.reserve(requestedLayers.size());

    auto tryEnable = [&](const std::string_view name) {
        if (availableLayerNames.contains(name)) {
            enabledLayers.push_back(name.data());
            m_enabledLayers.emplace(name);
        } else {
            Log::Debug("System doesn't have vulkan layer \"{}\"", name);
        }
    };

#ifdef DEBUG
    tryEnable("VK_LAYER_KHRONOS_validation");
#endif

    for (const std::string_view layerName : requestedLayers) {
        if (!m_enabledLayers.contains(layerName)) {
            tryEnable(layerName);
        }
    }

    return enabledLayers;
}

std::vector<const char*> CInstance::EnableExtensions(const std::span<RequestedExtension> requestedExtensions) {
    static const std::vector<vk::ExtensionProperties> extensionProps = m_context.enumerateInstanceExtensionProperties();
    static const std::unordered_set<std::string_view> availableExtensionNames =
        extensionProps
        | std::views::transform([&](const vk::ExtensionProperties& extension) -> std::string_view { return extension.extensionName; })
        | std::ranges::to<std::unordered_set>();

    std::vector<const char*> enabledExtensions;
    enabledExtensions.reserve(requestedExtensions.size() + 1);

    std::vector<RequestedExtension> missingExtensions;
    missingExtensions.reserve(4);

    m_enabledExtensions.reserve(requestedExtensions.size());

    auto tryEnable = [&](const std::string_view name, ExtensionRequirement requirement) {
        if (availableExtensionNames.contains(name)) {
            enabledExtensions.push_back(name.data());
            m_enabledExtensions.emplace(name);
        } else {
            missingExtensions.emplace_back(name, requirement);
        }
    };

#ifdef DEBUG
    tryEnable(vk::EXTDebugUtilsExtensionName, ExtensionRequirement::Optional);
#endif

    for (const auto& [extensionName, requirement] : requestedExtensions) {
        if (!m_enabledExtensions.contains(extensionName)) {
            tryEnable(extensionName, requirement);
        }
    }

    // Search extension in layers
    // Useful for android old drivers
    if (!missingExtensions.empty()) {
        for (const std::string_view layerName : m_enabledLayers) {
            const std::vector<vk::ExtensionProperties> availableExtensions =
                m_context.enumerateInstanceExtensionProperties(std::string { layerName });

            std::erase_if(missingExtensions, [&](const RequestedExtension& extension) {
                const auto [extensionName, requirement] = extension;
                if (HasExtension(availableExtensions, extensionName)) {
                    enabledExtensions.push_back(extensionName.data());
                    m_enabledExtensions.emplace(extensionName);
                    return true;
                }
                if (requirement == ExtensionRequirement::Optional)
                    return true;
                return false;
            });
        }
    }

    if (!missingExtensions.empty()) {
        std::string error = "System doesn't have required instance extensions:\n";
        for (const auto [name, _] : missingExtensions) {
            error += "\t";
            error += name;
            error += "\n";
        }
        throw std::runtime_error(error);
    }

    return enabledExtensions;
}

void CInstance::QueryPhysicalDevices() {
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
