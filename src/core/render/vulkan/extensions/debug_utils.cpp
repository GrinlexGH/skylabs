#include "debug_utils.hpp"

#include "console.hpp"

namespace {
vk::Bool32 DebugCallback(
    vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    [[maybe_unused]] vk::DebugUtilsMessageTypeFlagsEXT messageTypes,
    const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
    [[maybe_unused]] void* pUserData
) {
    switch (messageSeverity) {
        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose:
        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo:
            Msg("{}", pCallbackData->pMessage);
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

namespace Vulkan {
CDebugUtils::CDebugUtils(const vk::Instance& instance) {
    m_messenger = instance.createDebugUtilsMessengerEXT(CreateInfo());
}

void CDebugUtils::Destroy(const vk::Instance& instance) {
    instance.destroyDebugUtilsMessengerEXT(m_messenger);
    m_messenger = nullptr;
}

vk::DebugUtilsMessengerCreateInfoEXT& CDebugUtils::CreateInfo() {
    static vk::DebugUtilsMessengerCreateInfoEXT debugUtilsCreateInfo {};

    debugUtilsCreateInfo.messageSeverity =
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;

    debugUtilsCreateInfo.messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
            vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
            vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
            vk::DebugUtilsMessageTypeFlagBitsEXT::eDeviceAddressBinding;

    debugUtilsCreateInfo.pfnUserCallback =
            reinterpret_cast<vk::PFN_DebugUtilsMessengerCallbackEXT>(DebugCallback);

    return debugUtilsCreateInfo;
}
}
