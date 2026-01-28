#pragma once

#ifdef VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

extern "C" {
    VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetInstanceProcAddr(VkInstance instance, const char* pName);
    VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetDeviceProcAddr(VkDevice device, const char* pName);
    VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateInstanceVersion(uint32_t* pApiVersion);
    VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateInstanceExtensionProperties(const char* pLayerName, uint32_t* pPropertyCount, VkExtensionProperties* pProperties);
    VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateDeviceExtensionProperties(VkPhysicalDevice physicalDevice, const char* pLayerName, uint32_t* pPropertyCount, VkExtensionProperties* pProperties);
    VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceFeatures2(VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures2* pFeatures);
    VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceProperties2(VkPhysicalDevice physicalDevice, VkPhysicalDeviceProperties2* pProperties);
    VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceFormatProperties2(VkPhysicalDevice physicalDevice, VkFormat format, VkFormatProperties2* pFormatProperties);
    VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceQueueFamilyProperties2(VkPhysicalDevice physicalDevice, uint32_t* pQueueFamilyPropertyCount, VkQueueFamilyProperties2* pQueueFamilyProperties);
    VKAPI_ATTR VkResult VKAPI_CALL vkCreateInstance(const VkInstanceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkInstance* pInstance);
    VKAPI_ATTR VkResult VKAPI_CALL vkCreateDevice(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDevice* pDevice);
}
#endif

#include <vulkan/vulkan_profiles.hpp>


namespace Vulkan {
class CProfile
{
public:
    struct CProfileMeta {
        const char* m_name = nullptr;
        std::uint32_t m_specVersion = 0;
        std::uint32_t m_minApiVersion = 0;
    };

    enum class Profiles
    {
        eRoadmap2024,
        eRoadmap2022,
        eAndroidBaseline2022,
        eAndroidBaseline2021
    };

    explicit CProfile(std::nullptr_t) {}
    explicit CProfile(Profiles profile);

    void CheckInstanceSupport() const;
    bool CheckPhysicalDeviceSupport(VkInstance instance, VkPhysicalDevice physicalDevice) const;
    VkInstance CreateInstance(const VkInstanceCreateInfo& instanceCreateInfo) const;
    VkDevice CreateDevice(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo& deviceCreateInfo) const;
    std::uint32_t GetAPIVersion() const;
    std::vector<VkExtensionProperties> GetInstanceExtensions() const;
    std::vector<VkExtensionProperties> GetDeviceExtensions() const;

private:
    CProfileMeta m_currentProfile = {};
};
}
