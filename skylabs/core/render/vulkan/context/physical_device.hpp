#pragma once
#include <vulkan/vulkan_raii.hpp>

namespace Vulkan {
class CPhysicalDevice
{
public:
    explicit CPhysicalDevice(std::nullptr_t) {}
    explicit CPhysicalDevice(vk::raii::PhysicalDevice physicalDevice);

    [[nodiscard]] const vk::raii::PhysicalDevice& operator*() const noexcept { return m_handle; }
    [[nodiscard]] const vk::raii::PhysicalDevice* operator->() const noexcept { return &m_handle; }

private:
    vk::raii::PhysicalDevice m_handle { nullptr };
};
}
