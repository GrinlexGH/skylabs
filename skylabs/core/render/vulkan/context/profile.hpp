#pragma once
#include <skylabs/core/pch.hpp>

namespace Vulkan {
class CProfile
{
public:
    enum class Profile : std::uint8_t
    {
        eRoadmap2026,
        eRoadmap2024,
        eRoadmap2022
    };

    explicit CProfile(std::nullptr_t) {}
    explicit CProfile(Profile profile);

    void CheckInstanceSupport() const;
    [[nodiscard]] bool CheckPhysicalDeviceSupport(VkInstance instance, const vk::raii::PhysicalDevice& physicalDevice) const;
    [[nodiscard]] VkInstance CreateInstance(const VkInstanceCreateInfo& instanceCreateInfo) const;
    [[nodiscard]] VkDevice CreateDevice(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo& deviceCreateInfo) const;
    [[nodiscard]] std::uint32_t GetAPIVersion() const;
    [[nodiscard]] std::vector<VkExtensionProperties> GetInstanceExtensions() const;
    [[nodiscard]] std::vector<VkExtensionProperties> GetDeviceExtensions() const;
    [[nodiscard]] Profile GetCurrentProfile() const { return m_profile; }

private:
    Profile m_profile;

    [[nodiscard]] VpProfileProperties GenerateProperties() const;
};
}
