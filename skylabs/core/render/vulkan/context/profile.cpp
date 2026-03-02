#include <skylabs/core/render/vulkan/context/profile.hpp>
#include <skylabs/public/logging.hpp>

#include <frozen/map.h>

#include <algorithm>
#include <ranges>
#include <cstring>

struct ProfileMeta {
    const char* m_name = nullptr;
    std::uint32_t m_specVersion = 0;
    std::uint32_t m_minApiVersion = 0;
};

namespace Vulkan {
constexpr frozen::map<CProfile::Profile, ProfileMeta, 3> g_profileMap = {
    {
        CProfile::Profile::eRoadmap2026,
        ProfileMeta {
            .m_name = VP_KHR_ROADMAP_2026_NAME,
            .m_specVersion = VP_KHR_ROADMAP_2026_SPEC_VERSION,
            .m_minApiVersion = VP_KHR_ROADMAP_2026_MIN_API_VERSION
        }
    },
    {
        CProfile::Profile::eRoadmap2024,
        ProfileMeta {
            .m_name = VP_KHR_ROADMAP_2024_NAME,
            .m_specVersion = VP_KHR_ROADMAP_2024_SPEC_VERSION,
            .m_minApiVersion = VP_KHR_ROADMAP_2024_MIN_API_VERSION
        }
    },
    {
        CProfile::Profile::eRoadmap2022,
        ProfileMeta {
            .m_name = VP_KHR_ROADMAP_2022_NAME,
            .m_specVersion = VP_KHR_ROADMAP_2022_SPEC_VERSION,
            .m_minApiVersion = VP_KHR_ROADMAP_2022_MIN_API_VERSION
        }
    }
};

CProfile::CProfile(const Profile profile) : m_profile(profile) {}

void CProfile::CheckInstanceSupport() const {
    VpProfileProperties currentProfile = GenerateProperties();

    vk::Bool32 profileSupported;
    if (vpGetInstanceProfileSupport(nullptr, nullptr, &currentProfile, &profileSupported) != VK_SUCCESS) {
        throw std::runtime_error("Failed to get vulkan profile");
    }

    if (!profileSupported) {
        throw std::runtime_error(fmt::format("The vulkan profile {} is not supported", currentProfile.profileName));
    }
}

bool CProfile::CheckPhysicalDeviceSupport(VkInstance instance, const vk::raii::PhysicalDevice& physicalDevice) const {
    VpProfileProperties currentProfile = GenerateProperties();

    vk::Bool32 profileSupported;
    if (vpGetPhysicalDeviceProfileSupport(nullptr, instance, *physicalDevice, &currentProfile, &profileSupported) != VK_SUCCESS) {
        throw std::runtime_error("Cannot get physical device profile supported");
    }

    auto getFeatures = [&]<typename T>() {
        if (physicalDevice.getDispatcher()->vkGetPhysicalDeviceFeatures2) {
            return physicalDevice.getFeatures2<vk::PhysicalDeviceFeatures2, T>().template get<T>();
        }
        if (physicalDevice.getDispatcher()->vkGetPhysicalDeviceFeatures2KHR) {
            return physicalDevice.getFeatures2KHR<vk::PhysicalDeviceFeatures2, T>().template get<T>();
        }
        return T {};
    };

    if (m_profile == Profile::eRoadmap2022) {
        const auto features11 = getFeatures.operator()<vk::PhysicalDeviceVulkan11Features>();
        if (features11.shaderDrawParameters != vk::True) {
            Log::Warning("Application needs shaderDrawParameter feature!");
            return false;
        }
    }

    return profileSupported;
}

VkInstance CProfile::CreateInstance(const VkInstanceCreateInfo& instanceCreateInfo) const {
    VpProfileProperties currentProfile = GenerateProperties();

    VpInstanceCreateInfo vpCreateInfo {};
    vpCreateInfo.pCreateInfo = &instanceCreateInfo;
    vpCreateInfo.enabledFullProfileCount = 1;
    vpCreateInfo.pEnabledFullProfiles = &currentProfile;

    VkInstance instance = VK_NULL_HANDLE;
    if (vpCreateInstance(nullptr, &vpCreateInfo, nullptr, &instance) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create instance");
    }

    return instance;
}

VkDevice CProfile::CreateDevice(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo& deviceCreateInfo) const {
    VpProfileProperties currentProfile = GenerateProperties();

    VpDeviceCreateInfo vpCreateInfo {};
    vpCreateInfo.pCreateInfo = &deviceCreateInfo;
    vpCreateInfo.enabledFullProfileCount = 1;
    vpCreateInfo.pEnabledFullProfiles = &currentProfile;
    vpCreateInfo.flags = VP_DEVICE_CREATE_DISABLE_ROBUST_ACCESS;

    VkDevice device = VK_NULL_HANDLE;
    if (vpCreateDevice(nullptr, physicalDevice, &vpCreateInfo, nullptr, &device) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create device");
    }

    return device;
}

std::uint32_t CProfile::GetAPIVersion() const {
    VpProfileProperties currentProfile = GenerateProperties();
    return vpGetProfileAPIVersion(nullptr, &currentProfile);
}

std::vector<VkExtensionProperties> CProfile::GetInstanceExtensions() const {
    VpProfileProperties currentProfile = GenerateProperties();

    std::uint32_t extensionCount;
    if (vpGetProfileInstanceExtensionProperties(nullptr, &currentProfile, nullptr, &extensionCount, nullptr) != VK_SUCCESS) {
        throw std::runtime_error("Failed to get instance extensions count");
    }

    std::vector<VkExtensionProperties> extensions(extensionCount);
    if (vpGetProfileInstanceExtensionProperties(nullptr, &currentProfile, nullptr, &extensionCount, extensions.data()) != VK_SUCCESS) {
        throw std::runtime_error("Failed to get instance extensions");
    }

    return extensions;
}

std::vector<VkExtensionProperties> CProfile::GetDeviceExtensions() const {
    VpProfileProperties currentProfile = GenerateProperties();

    std::uint32_t extensionCount;
    if (vpGetProfileDeviceExtensionProperties(nullptr, &currentProfile, nullptr, &extensionCount, nullptr) != VK_SUCCESS) {
        throw std::runtime_error("Failed to get device extensions count");
    }

    std::vector<VkExtensionProperties> extensions(extensionCount);
    if (vpGetProfileDeviceExtensionProperties(nullptr, &currentProfile, nullptr, &extensionCount, extensions.data()) != VK_SUCCESS) {
        throw std::runtime_error("Failed to get device extensions");
    }

    return extensions;
}

VpProfileProperties CProfile::GenerateProperties() const {
    ProfileMeta meta = g_profileMap.at(m_profile);

    VpProfileProperties currentProfile {
        .profileName = {},
        .specVersion = meta.m_specVersion
    };

    std::strncpy(currentProfile.profileName, meta.m_name, sizeof(currentProfile.profileName));

    return currentProfile;
}
}

extern "C" {
VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetInstanceProcAddr(VkInstance instance, const char* pName) {
    return VULKAN_HPP_DEFAULT_DISPATCHER.vkGetInstanceProcAddr(instance, pName);
}

VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetDeviceProcAddr(VkDevice device, const char* pName) {
    return VULKAN_HPP_DEFAULT_DISPATCHER.vkGetDeviceProcAddr(device, pName);
}

VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateInstanceVersion(uint32_t* pApiVersion) {
    return VULKAN_HPP_DEFAULT_DISPATCHER.vkEnumerateInstanceVersion(pApiVersion);
}

VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateInstanceExtensionProperties(const char* pLayerName, uint32_t* pPropertyCount, VkExtensionProperties* pProperties) {
    return VULKAN_HPP_DEFAULT_DISPATCHER.vkEnumerateInstanceExtensionProperties(pLayerName, pPropertyCount, pProperties);
}

VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateDeviceExtensionProperties(VkPhysicalDevice physicalDevice, const char* pLayerName, uint32_t* pPropertyCount, VkExtensionProperties* pProperties) {
    return VULKAN_HPP_DEFAULT_DISPATCHER.vkEnumerateDeviceExtensionProperties(physicalDevice, pLayerName, pPropertyCount, pProperties);
}

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceFeatures2(VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures2* pFeatures) {
    VULKAN_HPP_DEFAULT_DISPATCHER.vkGetPhysicalDeviceFeatures2(physicalDevice, pFeatures);
}

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceProperties2(VkPhysicalDevice physicalDevice, VkPhysicalDeviceProperties2* pProperties) {
    VULKAN_HPP_DEFAULT_DISPATCHER.vkGetPhysicalDeviceProperties2(physicalDevice, pProperties);
}

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceFormatProperties2(VkPhysicalDevice physicalDevice, const VkFormat format, VkFormatProperties2* pFormatProperties) {
    VULKAN_HPP_DEFAULT_DISPATCHER.vkGetPhysicalDeviceFormatProperties2(physicalDevice, format, pFormatProperties);
}

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceQueueFamilyProperties2(VkPhysicalDevice physicalDevice, uint32_t* pQueueFamilyPropertyCount, VkQueueFamilyProperties2* pQueueFamilyProperties) {
    VULKAN_HPP_DEFAULT_DISPATCHER.vkGetPhysicalDeviceQueueFamilyProperties2(physicalDevice, pQueueFamilyPropertyCount, pQueueFamilyProperties);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateInstance(const VkInstanceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkInstance* pInstance) {
    return VULKAN_HPP_DEFAULT_DISPATCHER.vkCreateInstance(pCreateInfo, pAllocator, pInstance);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateDevice(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDevice* pDevice) {
    return VULKAN_HPP_DEFAULT_DISPATCHER.vkCreateDevice(physicalDevice, pCreateInfo, pAllocator, pDevice);
}
}
