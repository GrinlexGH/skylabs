#include "physical_device.hpp"

namespace Vulkan {
CPhysicalDevice::CPhysicalDevice(vk::raii::PhysicalDevice&& physicalDevice) :
    m_handle(std::move(physicalDevice)),
    m_properties(m_handle.getProperties()),
    m_features(m_handle.getFeatures()),
    m_queueFamilies(m_handle.getQueueFamilyProperties()),
    m_extensions(m_handle.enumerateDeviceExtensionProperties())
{
    Log::Info("Found GPU: {}", m_properties.deviceName.data());
}
}
