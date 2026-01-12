#include <skylabs/core/render/vulkan/context/instance.hpp>
#include <skylabs/core/render/vulkan/context/extensions.hpp>
#include <skylabs/public/logging.hpp>
#include "project_info.hpp"

#include <fmt/ranges.h>

#include <ranges>

namespace {
#ifdef DEBUG
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
            Log::Error("{}", pCallbackData->pMessage);
            break;
    }

    return vk::False;
}
#endif
}

namespace Vulkan {
CInstance::CInstance(
    const vk::raii::Context& context,
    const std::uint32_t apiVersion,
    const std::span<CRequestedExtension> requestedExtensions
) {
    // Collect all available global extensions
#ifndef DEBUG
    const
#endif
    std::vector<vk::ExtensionProperties> globalAvailableExtensions = context.enumerateInstanceExtensionProperties();

    // Enable validation layer
    std::vector<const char*> enabledLayers {};
#ifdef DEBUG
    {
        const std::vector<vk::LayerProperties> availableLayers = context.enumerateInstanceLayerProperties();

        constexpr auto validationLayer = "VK_LAYER_KHRONOS_validation";
        const bool validationAvailable = std::ranges::any_of(availableLayers, [&](const vk::LayerProperties& l) {
            return std::string_view { validationLayer } == l.layerName;
        });

        if (validationAvailable) {
            Log::Debug("Enabling {}", validationLayer);
            enabledLayers.push_back(validationLayer);

            // If VK_LAYER_KHRONOS_validation is enabled, extensions from it are also available
            std::vector<vk::ExtensionProperties> validationLayerExtensions = context.enumerateInstanceExtensionProperties(std::string { validationLayer });
            globalAvailableExtensions.insert(globalAvailableExtensions.end(), validationLayerExtensions.begin(), validationLayerExtensions.end());
        }
    }
#endif

    const auto isExtensionAvailable = [&](const std::string_view name) {
        return std::ranges::any_of(globalAvailableExtensions, [&](const vk::ExtensionProperties& ext) { return ext.extensionName == name; });
    };

    // Enable extensions
    void* pNext = nullptr;

#ifdef DEBUG
    // Debug utils messenger is the only object that need to be created in instance
    bool isDebugUtilsAvailable = false;
    vk::DebugUtilsMessengerCreateInfoEXT debugUtilsCreateInfo {};
    if (isExtensionAvailable(vk::EXTDebugUtilsExtensionName)) {
        isDebugUtilsAvailable = true;

        m_activeExtensions.emplace_back(vk::EXTDebugUtilsExtensionName);
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

        Utils::AppendToPNextChain(pNext, &debugUtilsCreateInfo);
    }
#endif

    // Process requested extensions
    std::vector<std::string_view> missingExtensions {};
    for (const auto& [name, requirement] : requestedExtensions) {
        if (isExtensionAvailable(name)) {
            m_activeExtensions.emplace_back(name);
        } else {
            if (requirement == ::Utils::Requirement::eRequired) {
                missingExtensions.push_back(name);
            } else {
                Log::Debug("Optional extension {} is not supported", name);
            }
        }
    }

    // Some required extensions are missing...
    if (!missingExtensions.empty()) {
        throw std::runtime_error(
            fmt::format("System doesn't have required instance extensions:\n    {}", fmt::join(missingExtensions, "\n    "))
        );
    }

    // For instance creation
    std::vector<const char*> enabledExtensions {};
    enabledExtensions.reserve(m_activeExtensions.size());
    for (const auto& ext : m_activeExtensions) {
        enabledExtensions.push_back(ext.c_str());
    }

    vk::ApplicationInfo applicationInfo {};
    applicationInfo.pApplicationName = Skylabs::GAME_NAME;
    applicationInfo.applicationVersion = vk::makeApiVersion(0, Skylabs::VERSION_MAJOR, Skylabs::VERSION_MINOR, Skylabs::VERSION_PATCH);
    applicationInfo.pEngineName = Skylabs::NAME;
    applicationInfo.engineVersion = vk::makeApiVersion(0, Skylabs::VERSION_MAJOR, Skylabs::VERSION_MINOR, Skylabs::VERSION_PATCH);
    applicationInfo.apiVersion = m_apiVersion = apiVersion;

    vk::InstanceCreateInfo instanceCreateInfo {};
    instanceCreateInfo.pNext = pNext;
    instanceCreateInfo.pApplicationInfo = &applicationInfo;
    instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(enabledLayers.size());
    instanceCreateInfo.ppEnabledLayerNames = !enabledLayers.empty() ? enabledLayers.data() : nullptr;
    instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(enabledExtensions.size());
    instanceCreateInfo.ppEnabledExtensionNames = !enabledExtensions.empty() ? enabledExtensions.data() : nullptr;

    m_handle = vk::raii::Instance { context, instanceCreateInfo };
    VULKAN_HPP_DEFAULT_DISPATCHER.init(*m_handle);

#ifdef DEBUG
    if (isDebugUtilsAvailable) {
        m_debugUtilsMessenger = vk::raii::DebugUtilsMessengerEXT { m_handle, debugUtilsCreateInfo };
    }
#endif
}
}
