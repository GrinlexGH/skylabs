#include <skylabs/core/render/vulkan/context/instance.hpp>

#include <skylabs/core/render/vulkan/context/physical_device.hpp>

#include <skylabs/public/project_info.hpp>

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
    const std::unordered_map<const char*, bool>& extensions,
    const std::vector<const char*>& layers
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
    m_enabledLayers.reserve(layers.size());
    std::vector<const char*> missingLayers;
    missingLayers.reserve(layers.size());

    // Currently (27.08.2025) validation layer cause sigsegv with gdb on windows, so don't use gdb on windows
#ifdef DEBUG
    if (!EnableLayer("VK_LAYER_KHRONOS_validation", m_enabledLayers)) {
        missingLayers.push_back("VK_LAYER_KHRONOS_validation");
    }
#endif

    for (const auto& name : layers) {
        if (!EnableLayer(name, m_enabledLayers)) {
            missingLayers.push_back(name);
        }
    }

    if (!missingLayers.empty()) {
        std::string error;
        error.reserve(missingLayers.size() * 20 + 40);
        error += "System doesn't have vulkan layers:\n";
        for (const char* const name : missingLayers) {
            error += '\t';
            error += name;
            error += '\n';
        }
        Log::Debug("{}", error);
    }

    //====================
#ifdef DEBUG
    const bool isDebugUtilsAvailable = EnableExtension(vk::EXTDebugUtilsExtensionName);
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
        std::string error;
        error.reserve(missingExtensions.size() * 20 + 50);
        error += "System doesn't have required instance extensions:\n";
        for (const auto name : missingExtensions) {
            error += '\t';
            error += name;
            error += '\n';
        }
        throw std::runtime_error(error);
    }

    //====================
    void* pNext = nullptr;

#ifdef DEBUG
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
    createInfo.ppEnabledExtensionNames = m_enabledExtensions.data();
    createInfo.enabledLayerCount = static_cast<uint32_t>(m_enabledLayers.size());
    createInfo.ppEnabledLayerNames = m_enabledLayers.data();
    createInfo.pNext = pNext;

    m_handle = m_context.createInstance(createInfo);
    VULKAN_HPP_DEFAULT_DISPATCHER.init(*m_handle);

    //====================
#ifdef DEBUG
    if (isDebugUtilsAvailable) {
        m_debugUtilsMessenger = m_handle.createDebugUtilsMessengerEXT(debugUtilsCreateInfo);
    }
#endif

    QueryPhysicalDevices();
}

auto CInstance::GetAvailableLayers() const -> std::vector<vk::LayerProperties> {
    const static std::vector<vk::LayerProperties> layers = m_context.enumerateInstanceLayerProperties();
    return layers;
}

auto CInstance::EnableExtension(const char* name) -> bool {
    static const std::vector<vk::ExtensionProperties>& availableGlobalExtensions = m_context.enumerateInstanceExtensionProperties();

    if (HasExtension(m_enabledExtensions, name)) {
        return true;
    }

    const auto tryEnable = [&](const std::vector<vk::ExtensionProperties>& availableExtensions) -> bool {
        if (HasExtension(availableExtensions, name)) {
            m_enabledExtensions.emplace_back(name);
            return true;
        }
        return false;
    };

    if (tryEnable(availableGlobalExtensions)) {
        return true;
    }

    // If any of the available layers provides this extension, enable it
    if (std::ranges::any_of(m_enabledLayers,
        [&](const char* layerName) {
            return tryEnable(m_context.enumerateInstanceExtensionProperties({ layerName }));
        }
    )) {
        return true;
    }

    return false;
}

auto CInstance::EnableLayer(const char* name, std::vector<const char*>& enabledLayers) const -> bool {
    if (HasLayer(enabledLayers, name)) {
        return true;
    }

    if (HasLayer(GetAvailableLayers(), name)) {
        enabledLayers.emplace_back(name);
        return true;
    }

    return false;
}

auto CInstance::QueryPhysicalDevices() -> void {
    vk::raii::PhysicalDevices physicalDevices { m_handle };
    if (physicalDevices.empty()) {
        throw std::runtime_error("Couldn't find a physical device that supports Vulkan!");
    }

    m_physicalDevices.reserve(physicalDevices.size());
    for (vk::raii::PhysicalDevice& physicalDevice : physicalDevices) {
        m_physicalDevices.emplace_back(std::move(physicalDevice));
    }
}
}
