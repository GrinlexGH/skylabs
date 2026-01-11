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

    [[nodiscard]] auto QueueFamilies() const noexcept -> const std::vector<vk::QueueFamilyProperties2KHR>& { return m_queueFamilies; }
    [[nodiscard]] auto AvailableExtensions() const noexcept -> const std::vector<vk::ExtensionProperties>& { return m_availableExtensions; }
    [[nodiscard]] auto Properties() const noexcept -> const vk::PhysicalDeviceProperties2KHR& { return m_properties; }
    [[nodiscard]] auto Features() const noexcept -> const auto& { return m_features; }

    [[nodiscard]] auto IsExtensionAvailable(const std::string_view name) const noexcept -> bool {
        return std::ranges::any_of(m_availableExtensions, [&](const vk::ExtensionProperties& ext) { return name == ext.extensionName; });
    }

private:
    vk::raii::PhysicalDevice m_handle { nullptr };
    std::vector<vk::QueueFamilyProperties2KHR> m_queueFamilies;
    std::vector<vk::ExtensionProperties> m_availableExtensions;
    vk::PhysicalDeviceProperties2KHR m_properties;
    vk::StructureChain<
        vk::PhysicalDeviceFeatures2KHR,
        vk::PhysicalDeviceVulkan11Features,
        vk::PhysicalDeviceVulkan12Features,
        vk::PhysicalDeviceVulkan13Features,
        vk::PhysicalDeviceDynamicRenderingFeaturesKHR
    > m_features;
};
}
