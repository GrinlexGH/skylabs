#include "instance.hpp"

#include "console.hpp"
#include "extensions/extensions.hpp"

#include <sstream>

namespace {
bool EnableExtension(const char* name, std::vector<const char*>& enabledExtensions) {
    static const std::vector<vk::ExtensionProperties>& availableExtensions = vk::enumerateInstanceExtensionProperties();

    if (HasExtension(availableExtensions, name)) {
        if (!HasExtension(enabledExtensions, name)) {
            enabledExtensions.push_back(name);
        }
    } else {
        return false;
    }

    return true;
}

bool EnableLayer(const char* name, std::vector<const char*>& enabledLayers) {
    static std::vector<vk::LayerProperties> availableLayers = vk::enumerateInstanceLayerProperties();

    if (HasLayer(availableLayers, name)) {
        if (!HasLayer(enabledLayers, name)) {
            enabledLayers.push_back(name);
        }
    }
    else {
        return false;
    }

    return true;
}

int GetDeviceTypeScore(const vk::PhysicalDeviceType type) {
    switch (type) {
        case vk::PhysicalDeviceType::eDiscreteGpu:
            return 5;
        case vk::PhysicalDeviceType::eIntegratedGpu:
            return 4;
        case vk::PhysicalDeviceType::eVirtualGpu:
            return 3;
        case vk::PhysicalDeviceType::eCpu:
            return 2;
        case vk::PhysicalDeviceType::eOther:
            return 1;
        default:
            return 0;
    }
}

bool IsDeviceSuitable(
    const vk::Instance& instance,
    const vk::PhysicalDevice& physicalDevice,
    const IVulkanWindow* const window
) {
    bool hasPresentQueue = false;
    bool hasGraphicsQueue = false;
    bool hasTransferQueue = false;

    for (std::uint32_t i = 0; const auto& queue : physicalDevice.getQueueFamilyProperties()) {
        if (window->CheckQueuePresentSupport(instance, physicalDevice, i)) {
            hasPresentQueue = true;
        }

        if (queue.queueFlags & vk::QueueFlagBits::eGraphics) {
            hasGraphicsQueue = true;
        }

        if (queue.queueFlags & vk::QueueFlagBits::eTransfer) {
            hasTransferQueue = true;
        }

        if (hasPresentQueue && hasGraphicsQueue && hasTransferQueue) {
            return true;
        }
        ++i;
    }

    return false;
}

vk::Bool32 DebugCallback(
    const vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    vk::DebugUtilsMessageTypeFlagsEXT /*messageTypes*/,
    const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* /*pUserData*/
) {
    switch (messageSeverity) {
        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose:
        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo:
            Msg("{}", pCallbackData->pMessage);
            break;
        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning:
            MsgD("\n{}", pCallbackData->pMessage);
            break;
        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eError:
            MsgE("\n{}\n", pCallbackData->pMessage);
            break;
    }

    return vk::False;
}
}

namespace Vulkan {
CInstance::CInstance(
    const char* gameName,
    const std::unordered_map<const char*, bool>& extensions,
    const std::vector<const char*>& layers
) {
    VULKAN_HPP_DEFAULT_DISPATCHER.init();

    m_apiVersion = vk::ApiVersion10;

    if (VULKAN_HPP_DEFAULT_DISPATCHER.vkEnumerateInstanceVersion) {
        m_apiVersion = vk::enumerateInstanceVersion();
    }

    Msg("Vulkan version: {}.{}.{}.{}",
        vk::apiVersionVariant(m_apiVersion),
        vk::apiVersionMajor(m_apiVersion),
        vk::apiVersionMinor(m_apiVersion),
        vk::apiVersionPatch(m_apiVersion)
    );

    //====================
    vk::ApplicationInfo appInfo {};
    appInfo.pApplicationName = gameName;
    appInfo.applicationVersion = 0;
    appInfo.pEngineName = "Skylabs";
    appInfo.engineVersion = 0;
    appInfo.apiVersion = m_apiVersion;

    //====================
#ifdef DEBUG
    const bool isDebugUtilsAvailable = EnableExtension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME, m_enabledExtensions);
#endif

    // if required extension is missing, put it into error message
    std::vector<const char*> missingExtensions {};
    missingExtensions.reserve(extensions.size());
    m_enabledExtensions.reserve(extensions.size());

    for (const auto& [name, required] : extensions) {
        if (required && !EnableExtension(name, m_enabledExtensions)) {
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
        for (const auto name : missingLayers) {
            error << '\t' << name << '\n';
        }
        MsgD("{}", error.str());
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
        vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
        vk::DebugUtilsMessageTypeFlagBitsEXT::eDeviceAddressBinding,

        DebugCallback
    };

    if (isDebugUtilsAvailable) {
        AppendToPNextChain(pNext, &debugUtilsCreateInfo);
    }
#endif

    //====================
    vk::InstanceCreateInfo instanceCreateInfo;
    instanceCreateInfo.pApplicationInfo = &appInfo;
    instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(m_enabledExtensions.size());
    instanceCreateInfo.ppEnabledExtensionNames = m_enabledExtensions.data();
    instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(enabledLayers.size());
    instanceCreateInfo.ppEnabledLayerNames = enabledLayers.data();
    instanceCreateInfo.pNext = pNext;

    m_handle = createInstance(instanceCreateInfo);
    VULKAN_HPP_DEFAULT_DISPATCHER.init(m_handle);

    //====================
#ifdef DEBUG
    if (isDebugUtilsAvailable) {
        m_debugUtilsMessenger = m_handle.createDebugUtilsMessengerEXT(debugUtilsCreateInfo);
    }
#endif

    QueryPhysicalDevices();
}

void CInstance::QueryPhysicalDevices() {
    std::vector<vk::PhysicalDevice> physical_devices = m_handle.enumeratePhysicalDevices();
    if (physical_devices.empty()) {
        throw std::runtime_error("Couldn't find a physical device that supports Vulkan!");
    }

    for (auto& physical_device : physical_devices) {
        m_physicalDevices.push_back(std::make_unique<CPhysicalDevice>(physical_device));
    }
}

const CPhysicalDevice& CInstance::GetSuitablePhysicalDevice(const IVulkanWindow* const window) const {
    const CPhysicalDevice* selectedDevice = VK_NULL_HANDLE;

    int deviceTypeScore = 0;
    for (const auto& physicalDevice : m_physicalDevices) {
        const int optionScore = GetDeviceTypeScore(physicalDevice->GetProperties().deviceType);
        if (IsDeviceSuitable(m_handle, physicalDevice->GetHandle(), window)) {
            if (optionScore > deviceTypeScore) {
                selectedDevice = physicalDevice.get();
                deviceTypeScore = optionScore;
            }
        }
    }

    if (selectedDevice == VK_NULL_HANDLE) {
        MsgW("No suitable GPU was found! Picking default GPU: {}", *m_physicalDevices[0]->GetProperties().deviceName);
        return *m_physicalDevices[0];
    }

    return *selectedDevice;
}

CInstance::~CInstance() {
    if (!m_handle) {
        return;
    }

#ifdef DEBUG
    if (m_debugUtilsMessenger != VK_NULL_HANDLE) {
        m_handle.destroyDebugUtilsMessengerEXT(m_debugUtilsMessenger);
    }
#endif

    m_handle.destroy();
}
}
