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

void AddDebugUtilsLayer(
    const std::vector<vk::LayerProperties>& availableLayers,
    const std::vector<vk::ExtensionProperties>& availableExtensions,
    CInstanceExtensionsCreateInfos& createInfos,
    std::vector<const char*>& enabledLayers,
    std::vector<const char*>& enabledExtensions,
    void*& pNextChain
) {
    if (!HasLayer(availableLayers, "VK_LAYER_KHRONOS_validation") &&
        !HasExtension(availableExtensions, VK_EXT_DEBUG_UTILS_EXTENSION_NAME)) {
        return;
    }

    enabledLayers.push_back("VK_LAYER_KHRONOS_validation");
    enabledExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    vk::DebugUtilsMessengerCreateInfoEXT debugUtilsCreateInfo;

    debugUtilsCreateInfo.messageSeverity =
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;

    debugUtilsCreateInfo.messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                                       vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                                       vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
                                       vk::DebugUtilsMessageTypeFlagBitsEXT::eDeviceAddressBinding;

    debugUtilsCreateInfo.pfnUserCallback =
        reinterpret_cast<PFN_vkDebugUtilsMessengerCallbackEXT>(DebugCallback);

    createInfos.m_debugUtilsMessengerCreateInfo = debugUtilsCreateInfo;
    pNextChain = AppendToPNextChain(pNextChain, &createInfos.m_debugUtilsMessengerCreateInfo);
}
