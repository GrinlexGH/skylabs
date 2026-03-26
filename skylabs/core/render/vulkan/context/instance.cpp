#include <skylabs/core/render/vulkan/context/instance.hpp>
#include <skylabs/core/render/vulkan/context/extensions.hpp>
#include <skylabs/public/logging.hpp>
#include "project_info.hpp"

#include <fmt/ranges.h>

namespace {
#ifdef DEBUG
constexpr auto validationLayerName = "VK_LAYER_KHRONOS_validation";

VKAPI_ATTR vk::Bool32 VKAPI_CALL DebugCallback(
    const vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
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
            Log::Error("{}\n", pCallbackData->pMessage);
            break;
    }

    return vk::False;
}
#endif

std::vector<const char*> SetupLayers([[maybe_unused]] const vk::raii::Context& context) {
    std::vector<const char*> enabledLayers {};

#if defined(DEBUG) && !defined(ARCH_32)
    for (const auto& layerProperties : context.enumerateInstanceLayerProperties()) {
        if (std::strcmp(layerProperties.layerName, validationLayerName) == 0) {
            enabledLayers.push_back(validationLayerName);
            break;
        }
    }
#endif

    return enabledLayers;
}

std::vector<vk::ExtensionProperties> GetAvailableExtensions(
    const vk::raii::Context& context,
    [[maybe_unused]] const std::vector<const char*>& enabledLayers
) {
    std::vector<vk::ExtensionProperties> globalAvailableExtensions = context.enumerateInstanceExtensionProperties();

#ifdef DEBUG
    for (const auto& layer : enabledLayers) {
        if (std::strcmp(layer, validationLayerName) == 0) {
            // If validation layer is enabled, extensions from it are also available
            std::vector<vk::ExtensionProperties> layerExtensions = context.enumerateInstanceExtensionProperties(std::string { validationLayerName });
            globalAvailableExtensions.insert(globalAvailableExtensions.end(), layerExtensions.begin(), layerExtensions.end());
            break;
        }
    }
#endif

    return globalAvailableExtensions;
}

std::unordered_map<std::string_view, bool> RequestExtensions(const std::span<const char* const> requiredExtensions) {
    std::unordered_map<std::string_view, bool> requestedExtensions;
    auto requestExtension = [&](const char* name, bool required) { requestedExtensions.try_emplace(name, required); };

    for (const auto& requiredExtension : requiredExtensions) {
        requestExtension(requiredExtension, true);
    }

#ifdef DEBUG
    requestExtension(vk::EXTDebugUtilsExtensionName, false);
#endif

    return requestedExtensions;
}
}

namespace Vulkan {
CInstance::CInstance(InstanceCreateInfo options) {
    VULKAN_HPP_DEFAULT_DISPATCHER.init();
    const vk::raii::Context context;

    // API version
    if (!context.getDispatcher()->vkEnumerateInstanceVersion) {
        throw std::runtime_error("Vulkan 1.0 is not supported");
    }

    const std::uint32_t currentApiVersion = context.enumerateInstanceVersion();

    if (currentApiVersion >= vk::ApiVersion14) {
        m_apiVersion = vk::ApiVersion14;
    } else if (currentApiVersion >= vk::ApiVersion13) {
        m_apiVersion = vk::ApiVersion13;
    } else {
        throw std::runtime_error("Vulkan below 1.3 is not supported");
    }

    // Extensions & layers
    const std::vector<const char*> enabledLayers = SetupLayers(context);
    const std::vector<const char*> enabledExtensions = SetupExtensions(context, options.m_requiredExtensions, enabledLayers);

    // Enable extensions
    Utils::PNextChain pNext;

#ifdef DEBUG
    bool isDebugUtilsEnabled = false;
    vk::DebugUtilsMessengerCreateInfoEXT debugUtilsCreateInfo {};
    if (options.m_setupDebugMessenger && IsExtensionEnabled(vk::EXTDebugUtilsExtensionName)) {
        debugUtilsCreateInfo.messageSeverity =
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;
        debugUtilsCreateInfo.messageType =
            vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
            vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
            vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;
        debugUtilsCreateInfo.pfnUserCallback = DebugCallback;

        pNext.Add(debugUtilsCreateInfo);

        isDebugUtilsEnabled = true;
    }
#endif

    vk::ApplicationInfo applicationInfo {};
    applicationInfo.pApplicationName = Skylabs::GAME_NAME;
    applicationInfo.applicationVersion = vk::makeApiVersion(0, Skylabs::VERSION_MAJOR, Skylabs::VERSION_MINOR, Skylabs::VERSION_PATCH);
    applicationInfo.pEngineName = Skylabs::NAME;
    applicationInfo.engineVersion = vk::makeApiVersion(0, Skylabs::VERSION_MAJOR, Skylabs::VERSION_MINOR, Skylabs::VERSION_PATCH);
    applicationInfo.apiVersion = m_apiVersion;

    vk::InstanceCreateInfo instanceCreateInfo {};
    instanceCreateInfo.pNext = pNext.m_head;
    instanceCreateInfo.pApplicationInfo = &applicationInfo;
    instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(enabledLayers.size());
    instanceCreateInfo.ppEnabledLayerNames = !enabledLayers.empty() ? enabledLayers.data() : nullptr;
    instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(enabledExtensions.size());
    instanceCreateInfo.ppEnabledExtensionNames = !enabledExtensions.empty() ? enabledExtensions.data() : nullptr;

    m_handle = vk::raii::Instance { context, instanceCreateInfo };
    VULKAN_HPP_DEFAULT_DISPATCHER.init(*m_handle);

#ifdef DEBUG
    if (isDebugUtilsEnabled) {
        m_debugUtilsMessenger = vk::raii::DebugUtilsMessengerEXT { m_handle, debugUtilsCreateInfo };
    }
#endif
}

std::vector<const char*> CInstance::SetupExtensions(
    const vk::raii::Context& context,
    const std::span<const char* const> requiredExtensions,
    const std::vector<const char*>& enabledLayers
) {
    const std::unordered_map<std::string_view, bool> requestedExtensions = RequestExtensions(requiredExtensions);

    // Find these extensions
    for (const auto& extension : GetAvailableExtensions(context, enabledLayers)) {
        if (requestedExtensions.contains(extension.extensionName)) {
            m_enabledExtensions.insert(extension.extensionName);
        }
    }

    std::vector<std::string_view> missingExtensions {};
    for (const auto& [name, required] : requestedExtensions) {
        if (required && !m_enabledExtensions.contains(name)) {
            missingExtensions.emplace_back(name);
        }
    }

    // Some required extensions are missing...
    if (!missingExtensions.empty()) {
        throw std::runtime_error(
            fmt::format("System doesn't have required instance extensions:\n    {}", fmt::join(missingExtensions, "\n    "))
        );
    }

    // Raw extension names
    std::vector<const char*> enabledExtensions {};
    enabledExtensions.reserve(m_enabledExtensions.size());
    for (const auto& ext : m_enabledExtensions) {
        enabledExtensions.push_back(ext.c_str());
    }

    return enabledExtensions;
}
}
