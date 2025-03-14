#pragma once
#include <unordered_map>
#include <memory>

#include "vulkan.hpp"
#include "vulkan_window.hpp"
#include "extensions/extensions.hpp"
#include "physical_device.hpp"

namespace Vulkan {
class CInstance
{
public:
    explicit CInstance(
        const char* gameName,
        const std::unordered_map<const char*, bool>& extensions,
        const std::vector<const char*>& layers = {}
    );
    CInstance(const CInstance&) = delete;
    CInstance(CInstance&&) = delete;
    CInstance& operator=(const CInstance&) = delete;
    CInstance& operator=(CInstance&&) = delete;
    ~CInstance();

    [[nodiscard]] vk::Instance GetHandle() const { return m_handle; }

    [[nodiscard]] bool IsExtensionEnabled(const std::string_view name) const { return HasExtension(m_enabledExtensions, name); }
    [[nodiscard]] std::uint32_t ApiVersion() const { return m_apiVersion; }

    const CPhysicalDevice& GetSuitablePhysicalDevice(const IVulkanWindow* window) const;

    explicit operator vk::Instance() const { return m_handle; }

private:
    void QueryPhysicalDevices();

    vk::Instance m_handle;

    std::vector<const char*> m_enabledExtensions;
    std::uint32_t m_apiVersion;

#ifdef DEBUG
    vk::DebugUtilsMessengerEXT m_debugUtilsMessenger = VK_NULL_HANDLE;
#endif

    std::vector<std::unique_ptr<CPhysicalDevice>> m_physicalDevices;
};
}
