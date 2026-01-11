#include <skylabs/core/render/vulkan/context/physical_device.hpp>

namespace Vulkan {
CPhysicalDevice::CPhysicalDevice(vk::raii::PhysicalDevice physicalDevice) :
    m_handle(std::move(physicalDevice)),
    m_queueFamilies(m_handle.getQueueFamilyProperties2KHR()),
    m_availableExtensions(m_handle.enumerateDeviceExtensionProperties()),
    m_properties(m_handle.getProperties2KHR()),
    m_features(m_handle.getFeatures2KHR<
        vk::PhysicalDeviceFeatures2KHR,
        vk::PhysicalDeviceVulkan11Features,
        vk::PhysicalDeviceVulkan12Features,
        vk::PhysicalDeviceVulkan13Features,
        vk::PhysicalDeviceDynamicRenderingFeaturesKHR>()
    ) {}
}
