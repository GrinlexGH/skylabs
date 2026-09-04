#pragma once
#include <skylabs/core/pch.hpp>
#include <skylabs/public/vulkan/os_connector.hpp>

namespace Vulkan {
class CInstance
{
public:
    explicit CInstance(std::nullptr_t) {}
    explicit CInstance(vk::Instance instance, std::vector<std::string>) {}
    explicit CInstance(const IOSConnector* osConnector = nullptr, bool setupDebugUtils = true);
    CInstance(CInstance&) = delete;
    CInstance(CInstance&&) = default;
    CInstance& operator=(CInstance&) = delete;
    CInstance& operator=(CInstance&&) = default;
    ~CInstance() = default;

    [[nodiscard]] const vk::raii::Instance& operator*() const noexcept { return m_handle; }
    [[nodiscard]] const vk::raii::Instance* operator->() const noexcept { return &m_handle; }
    [[nodiscard]] const vkb::Instance& VkbInstance() const noexcept { return m_vkbInstance; }
    [[nodiscard]] vkb::Instance& VkbInstance() noexcept { return m_vkbInstance; }

    [[nodiscard]] bool IsExtensionEnabled(const std::string_view name) const { return std::ranges::contains(m_enabledExtensions, name); }
    [[nodiscard]] std::uint32_t ApiVersion() const noexcept { return vk::ApiVersion10; }

private:
    [[nodiscard]] std::vector<const char*> SetupExtensions(
        const vk::raii::Context& context,
        const IOSConnector* osConnector,
        bool setupDebugUtils
    );

    vk::raii::Instance m_handle { nullptr };
    vkb::Instance m_vkbInstance;
#ifdef DEBUG
    vk::raii::DebugUtilsMessengerEXT m_debugUtilsMessenger { nullptr };
#endif

    std::vector<std::string> m_enabledExtensions;
};
}
