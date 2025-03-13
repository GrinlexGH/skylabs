#pragma once
#include "vulkan.hpp"
#include "vulkan_window.hpp"
#include "extensions/extensions.hpp"

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

    [[nodiscard]] const vk::PhysicalDeviceProperties& GetProperties() const { return m_properties; }
    [[nodiscard]] const vk::PhysicalDeviceFeatures& GetFeatures() const { return m_features; }
    [[nodiscard]] const std::vector<vk::ExtensionProperties>& GetExtensions() const { return m_extensions; }
    [[nodiscard]] const std::vector<vk::QueueFamilyProperties>& GetQueueFamilies() const { return m_queueFamilies; }

    [[nodiscard]] bool IsExtensionSupported(const char* name) const { return HasExtension(m_extensions, name); }

private:
    vk::PhysicalDevice m_handle {};

    vk::PhysicalDeviceProperties m_properties {};
    vk::PhysicalDeviceFeatures m_features {};
    std::vector<vk::QueueFamilyProperties> m_queueFamilies {};
    std::vector<vk::ExtensionProperties> m_extensions {};
};
}
