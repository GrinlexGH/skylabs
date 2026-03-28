#pragma once
#include <skylabs/core/render/vulkan/context/instance.hpp>
#include <skylabs/core/render/vulkan/platform/surface.hpp>

namespace Vulkan {
class CPhysicalDevice
{
public:
    explicit CPhysicalDevice(std::nullptr_t) {}
    explicit CPhysicalDevice(const CInstance& instance, const CSurface& surface);

    [[nodiscard]] const vk::raii::PhysicalDevice& operator*() const noexcept { return m_handle; }
    [[nodiscard]] const vk::raii::PhysicalDevice* operator->() const noexcept { return &m_handle; }
    [[nodiscard]] vkb::PhysicalDevice& VkbPhysicalDevice() noexcept { return m_vkbPhysicalDevice; }

private:
    vk::raii::PhysicalDevice m_handle { nullptr };
    vkb::PhysicalDevice m_vkbPhysicalDevice;
};
}
