#include <skylabs/core/render/vulkan/context/profile.hpp>

#include <fmt/format.h>
#include <frozen/map.h>
#include <vulkan/vulkan.hpp>

namespace Vulkan {
constexpr frozen::map<CProfile::Profiles, CProfile::CProfileMeta, 2> g_profileMap = {
    {
        CProfile::Profiles::eRoadmap2024,
        CProfile::CProfileMeta {
            .m_name = VP_KHR_ROADMAP_2024_NAME,
            .m_specVersion = VP_KHR_ROADMAP_2024_SPEC_VERSION,
            .m_minApiVersion = VP_KHR_ROADMAP_2024_MIN_API_VERSION
        }
    },
    {
        CProfile::Profiles::eRoadmap2022,
        CProfile::CProfileMeta {
            .m_name = VP_KHR_ROADMAP_2022_NAME,
            .m_specVersion = VP_KHR_ROADMAP_2022_SPEC_VERSION,
            .m_minApiVersion = VP_KHR_ROADMAP_2022_MIN_API_VERSION
        }
    }
};

CProfile::CProfile(const Profiles profile) : m_currentProfile(g_profileMap.at(profile)) {}

void CProfile::CheckInstanceSupport() const {
    VpProfileProperties currentProfile;
    strncpy_s(currentProfile.profileName, m_currentProfile.m_name, VP_MAX_PROFILE_NAME_SIZE);
    currentProfile.specVersion = m_currentProfile.m_specVersion;

    vk::Bool32 profileSupported;
    if (vpGetInstanceProfileSupport(nullptr, nullptr, &currentProfile, &profileSupported) != VK_SUCCESS) {
        throw std::runtime_error("Failed to get vulkan profile");
    }

    if (!profileSupported) {
        throw std::runtime_error(fmt::format("The vulkan profile {} is not supported", currentProfile.profileName));
    }
}

bool CProfile::CheckPhysicalDeviceSupport(VkInstance instance, VkPhysicalDevice physicalDevice) const {
    VpProfileProperties currentProfile;
    strncpy_s(currentProfile.profileName, m_currentProfile.m_name, VP_MAX_PROFILE_NAME_SIZE);
    currentProfile.specVersion = m_currentProfile.m_specVersion;

    vk::Bool32 profileSupported;
    if (vpGetPhysicalDeviceProfileSupport(nullptr, instance, physicalDevice, &currentProfile, &profileSupported) != VK_SUCCESS) {
        throw std::runtime_error("Cannot get physical device profile supported");
    }

    return profileSupported;
}

VkInstance CProfile::CreateInstance(const VkInstanceCreateInfo& instanceCreateInfo) const {
    VpProfileProperties currentProfile;
    strncpy_s(currentProfile.profileName, m_currentProfile.m_name, VP_MAX_PROFILE_NAME_SIZE);
    currentProfile.specVersion = m_currentProfile.m_specVersion;

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
    VpProfileProperties currentProfile;
    strncpy_s(currentProfile.profileName, m_currentProfile.m_name, VP_MAX_PROFILE_NAME_SIZE);
    currentProfile.specVersion = m_currentProfile.m_specVersion;

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
    VpProfileProperties currentProfile;
    strncpy_s(currentProfile.profileName, m_currentProfile.m_name, VP_MAX_PROFILE_NAME_SIZE);
    currentProfile.specVersion = m_currentProfile.m_specVersion;

    return vpGetProfileAPIVersion(nullptr, &currentProfile);
}

std::vector<VkExtensionProperties> CProfile::GetInstanceExtensions() const {
    VpProfileProperties currentProfile;
    strncpy_s(currentProfile.profileName, m_currentProfile.m_name, VP_MAX_PROFILE_NAME_SIZE);
    currentProfile.specVersion = m_currentProfile.m_specVersion;

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
    VpProfileProperties currentProfile;
    strncpy_s(currentProfile.profileName, m_currentProfile.m_name, VP_MAX_PROFILE_NAME_SIZE);
    currentProfile.specVersion = m_currentProfile.m_specVersion;

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
