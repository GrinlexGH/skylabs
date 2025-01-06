#include "device.hpp"

namespace Vulkan
{
void CDevice::Create(
    vk::PhysicalDevice physicalDevice,
    const std::vector<const char*>& requiredExtensions
) {
    vk::DeviceCreateInfo deviceCreateInfo;
    (void)requiredExtensions;
    deviceCreateInfo;

    m_handle = physicalDevice.createDevice(deviceCreateInfo);
}
}
