#pragma once
#include "extensions.hpp"

#include <memory>
#include <unordered_map>
#include <vulkan/vulkan_raii.hpp>

namespace Vulkan {
class CPhysicalDevice;

class CInstance
{
public:
    explicit CInstance(
        const std::unordered_map<const char*, bool>& extensions,
        const std::vector<const char*>& layers = { }
    );
    CInstance(const CInstance&) = delete;
    CInstance(CInstance&&) noexcept = default;
    CInstance& operator=(const CInstance&) = delete;
    CInstance& operator=(CInstance&&) noexcept = default;
    ~CInstance() = default;

    [[nodiscard]] const vk::raii::Instance& GetHandle() const { return m_handle; }

    [[nodiscard]] bool IsExtensionEnabled(const std::string_view name) const { return HasExtension(m_enabledExtensions, name); }

    [[nodiscard]] const std::vector<const char*>& GetEnabledExtensions() const { return m_enabledExtensions; }
    [[nodiscard]] std::uint32_t GetApiVersion() const { return m_apiVersion; }
    [[nodiscard]] const std::vector<std::unique_ptr<CPhysicalDevice>>& GetPhysicalDevices() const { return m_physicalDevices; }

private:
    bool EnableExtension(const char* name);
    bool EnableLayer(const char* name, std::vector<const char*>& enabledLayers) const;
    void QueryPhysicalDevices();

    vk::raii::Context m_context;
    vk::raii::Instance m_handle = VK_NULL_HANDLE;

    std::vector<const char*> m_enabledExtensions;
    std::uint32_t m_apiVersion = vk::ApiVersion10;

#ifdef DEBUG
    vk::raii::DebugUtilsMessengerEXT m_debugUtilsMessenger = VK_NULL_HANDLE;
#endif

    std::vector<std::unique_ptr<CPhysicalDevice>> m_physicalDevices;
};
}
