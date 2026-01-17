#include <skylabs/core/render/vulkan/context/physical_device.hpp>

namespace Vulkan {
CPhysicalDevice::CPhysicalDevice(vk::raii::PhysicalDevice physicalDevice) :
    m_handle(std::move(physicalDevice)) {}
}
