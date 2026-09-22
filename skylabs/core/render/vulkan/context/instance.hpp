#pragma once
#include <vulkan/vulkan_raii.hpp>

namespace sk::render::vulkan {
class Instance {
public:
    explicit Instance(std::nullptr_t) { }
    explicit Instance(const vk::raii::Context& context, const vk::Instance& instance,
                      const vk::DebugUtilsMessengerEXT& messenger,
                      std::vector<std::string>&& enabledExtensions)
        : m_handle(context, instance),
          m_debugUtilsMessenger(m_handle, messenger),
          m_enabledExtensions(std::move(enabledExtensions)) { }

    Instance(Instance&) = delete;
    Instance(Instance&&) = default;
    Instance& operator=(Instance&) = delete;
    Instance& operator=(Instance&&) = default;
    ~Instance() = default;

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
