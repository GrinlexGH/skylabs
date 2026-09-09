#pragma once
#include <skylabs/core/pch.hpp>

namespace Vulkan {
class CInstance {
public:
    explicit CInstance(std::nullptr_t) { }
    explicit CInstance(const vk::raii::Context& context, const vk::Instance& instance,
                       const vk::DebugUtilsMessengerEXT& messenger,
                       std::vector<std::string>&& enabledExtensions)
        : m_handle(context, instance),
          m_debugUtilsMessenger(m_handle, messenger),
          m_enabledExtensions(std::move(enabledExtensions)) { }

    CInstance(CInstance&) = delete;
    CInstance(CInstance&&) = default;
    CInstance& operator=(CInstance&) = delete;
    CInstance& operator=(CInstance&&) = default;
    ~CInstance() = default;

    [[nodiscard]] const vk::raii::Instance& operator*() const noexcept { return m_handle; }
    [[nodiscard]] const vk::raii::Instance* operator->() const noexcept { return &m_handle; }

    [[nodiscard]] bool IsExtensionEnabled(const std::string_view name) const {
        return std::ranges::contains(m_enabledExtensions, name);
    }

private:
    vk::raii::Instance m_handle { nullptr };
    vk::raii::DebugUtilsMessengerEXT m_debugUtilsMessenger { nullptr };

    std::vector<std::string> m_enabledExtensions;
};
}
