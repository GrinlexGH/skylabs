#pragma once
#include "extensions.hpp"

#include <unordered_map>
#include <vulkan/vulkan_raii.hpp>

namespace Vulkan {
class CPhysicalDevice;

class CInstance
{
public:
    explicit CInstance(std::nullptr_t);
    explicit CInstance(
        const std::unordered_map<const char*, bool>& extensions,
        const std::vector<const char*>& layers = {}
    );
    CInstance(const CInstance&) = delete;
    CInstance(CInstance&&) noexcept = default;
    CInstance& operator=(const CInstance&) = delete;
    CInstance& operator=(CInstance&&) noexcept = default;
    ~CInstance() = default;

    [[nodiscard]] auto GetHandle() const -> const vk::raii::Instance& { return m_handle; }

    [[nodiscard]] auto IsExtensionEnabled(const std::string_view name) const -> bool { return HasExtension(m_enabledExtensions, name); }

    [[nodiscard]] auto GetEnabledExtensions() const -> const std::vector<const char*>& { return m_enabledExtensions; }
    [[nodiscard]] auto GetApiVersion() const -> std::uint32_t { return m_apiVersion; }
    [[nodiscard]] auto GetPhysicalDevices() -> std::vector<CPhysicalDevice>& { return m_physicalDevices; }

private:
    [[nodiscard]] auto GetAvailableLayers() const -> std::vector<vk::LayerProperties>;
    auto EnableExtension(const char* name) -> bool;
    auto EnableLayer(const char* name, std::vector<const char*>& enabledLayers) const -> bool;
    auto QueryPhysicalDevices() -> void;

    vk::raii::Context m_context;
    vk::raii::Instance m_handle = nullptr;

    std::vector<const char*> m_enabledExtensions;
    std::vector<const char*> m_enabledLayers;
    std::uint32_t m_apiVersion = vk::ApiVersion10;

#ifdef DEBUG
    vk::raii::DebugUtilsMessengerEXT m_debugUtilsMessenger = nullptr;
#endif

    std::vector<CPhysicalDevice> m_physicalDevices;
};
}
