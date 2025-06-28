#include "physical_device.hpp"

namespace Vulkan {
CPhysicalDevice::CPhysicalDevice(const vk::PhysicalDevice& physicalDevice) :
    m_handle(physicalDevice),
    m_properties(m_handle.getProperties()),
    m_features(m_handle.getFeatures()),
    m_queueFamilies(m_handle.getQueueFamilyProperties()),
    m_extensions(m_handle.enumerateDeviceExtensionProperties()) {
    Log::Info("Found GPU: {}", m_properties.deviceName.data());
}
}
