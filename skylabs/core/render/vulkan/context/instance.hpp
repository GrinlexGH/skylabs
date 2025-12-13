#pragma once
#include <skylabs/core/render/vulkan/context/extensions.hpp>
#include <skylabs/public/string_utils.hpp>

#include <unordered_map>

#include <vulkan/vulkan_raii.hpp>

namespace Vulkan {
class CPhysicalDevice;

class CInstance
{
public:
    explicit CInstance(std::nullptr_t);
    explicit CInstance(
        const std::unordered_map<std::string_view, bool>& extensions,
        const std::vector<std::string_view>& layers = {}
    );
    CInstance(const CInstance&) = delete;
    CInstance(CInstance&&) noexcept = default;
    CInstance& operator=(const CInstance&) = delete;
    CInstance& operator=(CInstance&&) noexcept = default;
    ~CInstance() = default;

    auto operator*() const noexcept -> const vk::raii::Instance& { return m_handle; }
    [[nodiscard]] auto GetHandle() const -> const vk::raii::Instance& { return m_handle; }

    [[nodiscard]] auto IsExtensionEnabled(const std::string_view name) const -> bool { return m_enabledExtensions.contains(name); }

    [[nodiscard]] auto GetApiVersion() const -> std::uint32_t { return m_apiVersion; }
    [[nodiscard]] auto GetPhysicalDevices() -> std::vector<CPhysicalDevice>& { return m_physicalDevices; }

private:
    auto EnableLayers(const std::vector<std::string_view>& requestedLayers) -> std::vector<const char*>;
    auto EnableExtensions(const std::unordered_map<std::string_view, bool>& requestedExtensions) -> std::vector<const char*>;
    auto QueryPhysicalDevices() -> void;

    vk::raii::Context m_context;
    vk::raii::Instance m_handle = nullptr;

    UnorderedStringSet m_enabledExtensions;
    UnorderedStringSet m_enabledLayers;
    std::uint32_t m_apiVersion = vk::ApiVersion10;

#ifdef DEBUG
    vk::raii::DebugUtilsMessengerEXT m_debugUtilsMessenger = nullptr;
#endif

    std::vector<CPhysicalDevice> m_physicalDevices;
};
}
