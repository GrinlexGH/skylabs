#pragma once
#include <vulkan/vulkan_raii.hpp>

namespace Vulkan {
class CPhysicalDevice
{
public:
    explicit CPhysicalDevice(std::nullptr_t) {}
    explicit CPhysicalDevice(vk::raii::PhysicalDevice physicalDevice);

    [[nodiscard]] auto operator*() const noexcept -> const vk::raii::PhysicalDevice& { return m_handle; }
    [[nodiscard]] auto operator->() const noexcept -> const vk::raii::PhysicalDevice* { return &m_handle; }

private:
    vk::raii::PhysicalDevice m_handle { nullptr };
};
}
