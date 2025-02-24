#pragma once
#include "vulkan.hpp"
#include "vulkan_window.hpp"

namespace Vulkan {
class CPhysicalDevice
{
public:
    explicit CPhysicalDevice(const vk::PhysicalDevice& physicalDevice);
    CPhysicalDevice(const CPhysicalDevice&) = delete;
    CPhysicalDevice(CPhysicalDevice&&) = delete;
    CPhysicalDevice& operator=(const CPhysicalDevice&) = delete;
    CPhysicalDevice& operator=(CPhysicalDevice&&) = delete;
    ~CPhysicalDevice() = default;

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
