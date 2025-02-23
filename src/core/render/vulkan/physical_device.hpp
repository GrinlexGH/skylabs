#pragma once
#include "vulkan.hpp"
#include "vulkan_window.hpp"
#include "instance.hpp"

namespace Vulkan {
class CPhysicalDevice
{
public:
    void Pick(const CInstance& instance, const IVulkanWindow* window);
    void Set(const vk::PhysicalDevice& physicalDevice);

    [[nodiscard]] vk::PhysicalDevice GetHandle() const { return m_handle; }
    [[nodiscard]] vk::PhysicalDeviceProperties GetProperties() const { return m_properties; }
    [[nodiscard]] vk::PhysicalDeviceFeatures GetFeatures() const { return m_features; }
    [[nodiscard]] std::vector<vk::QueueFamilyProperties> GetQueueFamilyProperties() const { return m_queueFamilyProperties; }

private:
    vk::PhysicalDevice m_handle {};

    vk::PhysicalDeviceProperties m_properties {};
    vk::PhysicalDeviceFeatures m_features {};
    std::vector<vk::QueueFamilyProperties> m_queueFamilyProperties {};
};
}
