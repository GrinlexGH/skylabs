#pragma once
#include <skylabs/core/pch.hpp>
#include <skylabs/public/utils.hpp>

namespace Vulkan {
class CInstance
{
public:
    explicit CInstance(std::nullptr_t) {}
    explicit CInstance(bool setupDebugUtils = true);
    CInstance(CInstance&) = delete;
    CInstance(CInstance&&) = default;
    CInstance& operator=(CInstance&) = delete;
    CInstance& operator=(CInstance&&) = default;
    ~CInstance() = default;

    [[nodiscard]] const vk::raii::Instance& operator*() const noexcept { return m_handle; }
    [[nodiscard]] const vk::raii::Instance* operator->() const noexcept { return &m_handle; }
    [[nodiscard]] const vkb::Instance& VkbInstance() const noexcept { return m_vkbInstance; }
    [[nodiscard]] vkb::Instance& VkbInstance() noexcept { return m_vkbInstance; }

    [[nodiscard]] bool IsExtensionEnabled(const std::string_view name) const { return m_enabledExtensions.contains(name); }
    [[nodiscard]] std::uint32_t ApiVersion() const noexcept { return m_vkbInstance.api_version; }

private:
    [[nodiscard]] std::vector<const char*> SetupExtensions(const vk::raii::Context& context, bool setupDebugUtils);

    vk::raii::Instance m_handle { nullptr };
    vkb::Instance m_vkbInstance;

    Utils::StringUnorderedSet m_enabledExtensions;

#ifdef DEBUG
    vk::raii::DebugUtilsMessengerEXT m_debugUtilsMessenger { nullptr };
#endif
};
}
