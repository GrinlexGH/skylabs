#include "debug_messenger.hpp"

#include "extension_manager.hpp"
#include "console.hpp"

namespace
{
vk::Bool32 DebugCallback(
    vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    [[maybe_unused]] vk::DebugUtilsMessageTypeFlagsEXT messageTypes,
    const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
    [[maybe_unused]] void* pUserData
) {
    switch (messageSeverity) {
        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose:
        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo:
            Msg << pCallbackData->pMessage;
            break;
        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning:
            Warning("\n{}", pCallbackData->pMessage);
            break;
        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eError:
            Error("\n{}\n", pCallbackData->pMessage);
            break;
    }

    return vk::False;
}
}

bool PrepareDebugUtilsExtension(
    vk::DebugUtilsMessengerCreateInfoEXT& debugUtilsMessengerCreateInfo,
    std::vector<const char*>& enabledExtensions,
    std::vector<const char*>& enabledLayers,
    const std::vector<vk::ExtensionProperties>& availableExtensions,
    const std::vector<vk::LayerProperties>& availableLayers
) {
    if (!HasLayer(availableLayers, "VK_LAYER_KHRONOS_validation") &&
        !HasExtension(availableExtensions, VK_EXT_DEBUG_UTILS_EXTENSION_NAME)) {
        return false;
    }

    enabledLayers.push_back("VK_LAYER_KHRONOS_validation");
    enabledExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    debugUtilsMessengerCreateInfo.messageSeverity =
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;

    debugUtilsMessengerCreateInfo.messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                                                vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                                                vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
                                                vk::DebugUtilsMessageTypeFlagBitsEXT::eDeviceAddressBinding;

    debugUtilsMessengerCreateInfo.pfnUserCallback =
        reinterpret_cast<PFN_vkDebugUtilsMessengerCallbackEXT>(DebugCallback);

    return true;
}
