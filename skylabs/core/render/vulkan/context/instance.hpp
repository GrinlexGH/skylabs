#pragma once
#include <skylabs/public/string_utils.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <unordered_map>

namespace Vulkan {
class CInstance
{
public:
    explicit CInstance(std::nullptr_t) {}
    explicit CInstance(std::span<const char* const> requiredExtensions = {});
    CInstance(CInstance&) = delete;
    CInstance(CInstance&&) = default;
    CInstance& operator=(CInstance&) = delete;
    CInstance& operator=(CInstance&&) = default;
    ~CInstance() = default;

    [[nodiscard]] auto operator*() const noexcept -> const vk::raii::Instance& { return m_handle; }
    [[nodiscard]] auto operator->() const noexcept -> const vk::raii::Instance* { return &m_handle; }

    [[nodiscard]] auto IsExtensionEnabled(const std::string_view name) const -> bool { return m_enabledExtensions.contains(name); }

private:
    [[nodiscard]] auto SetupExtensions(
        const vk::raii::Context& context,
        std::span<const char* const> requiredExtensions,
        const std::vector<const char*>& enabledLayers
    ) -> std::vector<const char*>;

    vk::raii::Instance m_handle { nullptr };

    UnorderedStringSet m_enabledExtensions;

#ifdef DEBUG
    vk::raii::DebugUtilsMessengerEXT m_debugUtilsMessenger { nullptr };
#endif
};
}
