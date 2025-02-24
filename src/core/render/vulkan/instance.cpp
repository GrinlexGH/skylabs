#include "instance.hpp"

#include "console.hpp"
#include "extensions/extensions.hpp"
#include "extensions/debug_utils.hpp"

#include <sstream>

namespace {
bool EnableExtension(const char* name, std::vector<const char*>& enabledExtensions) {
    static std::vector<vk::ExtensionProperties> availableExtensions = vk::enumerateInstanceExtensionProperties();

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
    for (std::size_t i = 0; i < physicalDevice.getQueueFamilyProperties().size(); ++i) {
        if (window->CheckQueuePresentSupport(instance, physicalDevice, static_cast<std::uint32_t>(i))) {
            return true;
        }
    }

    return false;
}
}

namespace Vulkan {
CInstance::CInstance(
    const Config& config,
    const std::unordered_map<const char*, bool>& extensions,
    const std::vector<const char*>& layers
) {
    VULKAN_HPP_DEFAULT_DISPATCHER.init();

    std::uint32_t apiVersion = vk::ApiVersion10;

    if (VULKAN_HPP_DEFAULT_DISPATCHER.vkEnumerateInstanceVersion) {
        apiVersion = std::max(vk::enumerateInstanceVersion(), apiVersion);
    }

    Msg("Vulkan version: {}.{}.{}.{}",
        vk::apiVersionVariant(apiVersion),
        vk::apiVersionMajor(apiVersion),
        vk::apiVersionMinor(apiVersion),
        vk::apiVersionPatch(apiVersion)
    );

    //====================
    vk::ApplicationInfo appInfo {};
    appInfo.pApplicationName = config.m_gameName;
    appInfo.applicationVersion = config.m_gameVersion;
    appInfo.pEngineName = config.m_engineName;
    appInfo.engineVersion = config.m_engineBuild;
    appInfo.apiVersion = apiVersion;

    //====================
#ifdef DEBUG
    const bool isDebugUtilsAvailable = EnableExtension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME, m_enabledExtensions);
#endif

    // if required extension is missing, put it into error message
    std::vector<const char*> missingExtensions {};
    for (const auto& [name, required] : extensions) {
        if (const bool isAvailable = EnableExtension(name, m_enabledExtensions); required && !isAvailable) {
            missingExtensions.push_back(name);
        }
    }

    if (!missingExtensions.empty()) {
        std::ostringstream error;
        error << "System doesn't have required instance extensions:\n";
        for (const auto name : missingExtensions) {
            error << name << '\n';
        }
        throw std::runtime_error(error.str());
    }

    //====================
    std::vector<const char*> enabledLayers;
#ifdef DEBUG
    EnableLayer("VK_LAYER_KHRONOS_validation", enabledLayers);
#endif

    for (const auto& name : layers) {
        EnableLayer(name, enabledLayers);
    }

    //====================
    void* pNext = nullptr;

#ifdef DEBUG
    if (isDebugUtilsAvailable) {
        AppendToPNextChain(pNext, &CDebugUtils::CreateInfo());
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
        m_debugUtils = std::make_unique<CDebugUtils>(m_handle);
    }
#endif

    std::vector<vk::PhysicalDevice> physical_devices = m_handle.enumeratePhysicalDevices();
    if (physical_devices.empty()) {
        throw std::runtime_error("Couldn't find a physical device that supports Vulkan.");
    }

    for (auto& physical_device : physical_devices) {
        m_physicalDevices.push_back(std::make_unique<CPhysicalDevice>(physical_device));
    }
}

CPhysicalDevice& CInstance::GetSuitablePhysicalDevice(const IVulkanWindow* const window) const {
    CPhysicalDevice* selectedDevice = VK_NULL_HANDLE;

    int deviceTypeScore = 0;
    for (const auto& physicalDevice : m_physicalDevices) {

        Msg("Found device: {}", static_cast<const char*>(physicalDevice->GetProperties().deviceName));

        const int optionScore = GetDeviceTypeScore(physicalDevice->GetProperties().deviceType);
        if (IsDeviceSuitable(m_handle, physicalDevice->GetHandle(), window)) {
            if (optionScore > deviceTypeScore) {
                selectedDevice = physicalDevice.get();
                deviceTypeScore = optionScore;
            }
        }
    }

    if (selectedDevice == VK_NULL_HANDLE) {
        Warning("No suitable GPU was found! Picking default GPU.");
        return *m_physicalDevices[0];
    }

    return *selectedDevice;
}

CInstance::~CInstance() {
    if (!m_handle) {
        return;
    }

    m_debugUtils->Destroy(m_handle);

    m_handle.destroy();
}
}
