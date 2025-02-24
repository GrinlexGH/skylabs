#include "physical_device.hpp"

#include "queue_families.hpp"

namespace Vulkan {
CPhysicalDevice::CPhysicalDevice(const vk::PhysicalDevice& physicalDevice) :
    m_handle(physicalDevice),
    m_properties(m_handle.getProperties()),
    m_features(m_handle.getFeatures()),
    m_queueFamilyProperties(m_handle.getQueueFamilyProperties())
{}
}
