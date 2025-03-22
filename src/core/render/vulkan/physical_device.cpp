#include "physical_device.hpp"

namespace Vulkan {
CPhysicalDevice::CPhysicalDevice(const vk::PhysicalDevice& physicalDevice, const CInstance* instance) :
    m_handle(physicalDevice),
    m_instance(instance),
    m_properties(m_handle.getProperties()),
    m_features(m_handle.getFeatures()),
    m_queueFamilies(m_handle.getQueueFamilyProperties()),
    m_extensions(m_handle.enumerateDeviceExtensionProperties())
{
    Msg("Found GPU: {}", m_properties.deviceName.data());
}
}
