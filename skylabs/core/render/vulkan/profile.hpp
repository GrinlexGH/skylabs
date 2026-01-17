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

namespace Vulkan::Profile {
constexpr inline VpProfileProperties currentProfile = {
    .profileName = VP_KHR_ROADMAP_2024_NAME,
    .specVersion = VP_KHR_ROADMAP_2024_SPEC_VERSION
};

constexpr inline std::uint32_t currentProfileApiVersion = VP_KHR_ROADMAP_2024_MIN_API_VERSION;

void CheckInstanceSupport();
bool CheckPhysicalDeviceSupport(VkInstance instance, VkPhysicalDevice physicalDevice);
VkInstance CreateInstance(const VkInstanceCreateInfo& instanceCreateInfo);
VkDevice CreateDevice(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo& deviceCreateInfo);
std::uint32_t GetVersion();
std::vector<VkExtensionProperties> GetInstanceExtensions();
std::vector<VkExtensionProperties> GetDeviceExtensions();
}
