#pragma once
#include <flat_set>

#include <vulkan/vulkan_raii.hpp>

namespace sk::render::vulkan {
using ExtensionSet = std::flat_set<std::string, std::less<>>;

class Instance {
public:
    explicit Instance(std::nullptr_t) { }
    explicit Instance(const vk::raii::Context& context, const vk::Instance& instance,
                      const vk::DebugUtilsMessengerEXT& messenger, ExtensionSet&& enabledExtensions)
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
        return m_enabledExtensions.contains(name);
    }

private:
    vk::raii::Instance m_handle { nullptr };
    vk::raii::DebugUtilsMessengerEXT m_debugUtilsMessenger { nullptr };

    ExtensionSet m_enabledExtensions;
};
}
