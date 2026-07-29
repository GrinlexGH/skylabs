#include <skylabs/core/render/vulkan/context/instance.hpp>
#include <skylabs/public/logging.hpp>
#include "project_info.hpp"

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
            Log::Error("{}\n", pCallbackData->pMessage);
            break;
    }

    return vk::False;
}
#endif

auto GetAvailableExtensions(const vk::raii::Context& context) {
    std::vector<vk::ExtensionProperties> globalExtensions = context.enumerateInstanceExtensionProperties();

#if defined(DEBUG) && !defined(ARCH_32)
    for (auto& layer : context.enumerateInstanceLayerProperties()) {
        if (std::strcmp(layer.layerName, "VK_LAYER_KHRONOS_validation") != 0) {
            continue;
        }

        for (auto& ext : context.enumerateInstanceExtensionProperties(std::string { std::string_view { layer.layerName } })) {
            globalExtensions.push_back(ext);
        }
    }
#endif

    return globalExtensions;
}
}

namespace Vulkan {
CInstance::CInstance(const IOSConnector* osConnector, const bool setupDebugUtils) {
    const vk::raii::Context context { osConnector->GetVkGetInstanceProcAddr() };

    const std::vector<const char*> enabledExtensions = SetupExtensions(context, osConnector, setupDebugUtils);
    constexpr std::uint32_t appVersion = vk::makeApiVersion(0, Skylabs::VERSION_MAJOR, Skylabs::VERSION_MINOR, Skylabs::VERSION_PATCH);
    constexpr auto debugSeverity =
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;
    constexpr auto debugTypes =
        vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
        vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
        vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;

    vkb::InstanceBuilder instanceBuilder;
    instanceBuilder
        .set_app_name(Skylabs::GAME_NAME)
        .set_app_version(appVersion)
        .set_engine_name(Skylabs::NAME)
        .set_engine_version(appVersion)
        .set_minimum_instance_version(vk::ApiVersion13)
        .enable_extensions(enabledExtensions);

#ifdef DEBUG
    if (setupDebugUtils && m_enabledExtensions.contains(vk::EXTDebugUtilsExtensionName)) {
        instanceBuilder
#if !defined(ARCH_32)
        .request_validation_layers()
#endif
        .set_debug_callback(reinterpret_cast<PFN_vkDebugUtilsMessengerCallbackEXT>(reinterpret_cast<std::uintptr_t>(DebugCallback)))
        .set_debug_messenger_severity(static_cast<VkDebugUtilsMessageSeverityFlagsEXT>(debugSeverity))
        .set_debug_messenger_type(static_cast<VkDebugUtilsMessageTypeFlagsEXT>(debugTypes));
    }
#endif

    auto instanceResult = instanceBuilder.build();
    if (!instanceResult) {
        throw std::runtime_error(
            fmt::format("Failed to create vulkan instance ({}): {}, {}",
                vk::to_string(vk::Result { instanceResult.vk_result() }),
                instanceResult.error().message(),
                instanceResult.detailed_failure_reasons()
            )
        );
    }

    m_vkbInstance = instanceResult.value();

    m_handle = vk::raii::Instance { context, m_vkbInstance.instance };

#ifdef DEBUG
    m_debugUtilsMessenger = vk::raii::DebugUtilsMessengerEXT { m_handle, m_vkbInstance.debug_messenger };
#endif
}

std::vector<const char*> CInstance::SetupExtensions(
    const vk::raii::Context& context,
    const IOSConnector* surfaceProvider,
    [[maybe_unused]] const bool setupDebugUtils
) {
    boost::container::flat_set<std::string_view> requestedExtensions {
        vk::EXTSwapchainColorSpaceExtensionName
    };

#ifdef DEBUG
    if (setupDebugUtils) {
        requestedExtensions.emplace(vk::EXTDebugUtilsExtensionName);
    }
#endif

    if (surfaceProvider) {
        for (auto& ext : surfaceProvider->RequiredInstanceExtensions()) {
            requestedExtensions.emplace(ext);
        }
    }

    if (requestedExtensions.empty()) {
        return {};
    }

    // Find these extensions
    m_enabledExtensions.reserve(requestedExtensions.size());
    for (const auto& extension : GetAvailableExtensions(context)) {
        if (requestedExtensions.contains(std::string_view { extension.extensionName })) {
            m_enabledExtensions.emplace(extension.extensionName.data());
        }
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
