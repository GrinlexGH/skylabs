#pragma once
#include <skylabs/core/render/vulkan/context/profile.hpp>
#include <skylabs/public/string_utils.hpp>

namespace Vulkan {
struct InstanceCreateInfo
{
    std::span<const char* const> m_requiredExtensions {};
    bool m_setupDebugMessenger = true;
};

class CInstance
{
public:
    explicit CInstance(std::nullptr_t) {}
    explicit CInstance(CProfile profile, InstanceCreateInfo options = {});
    CInstance(CInstance&) = delete;
    CInstance(CInstance&&) = default;
    CInstance& operator=(CInstance&) = delete;
    CInstance& operator=(CInstance&&) = default;
    ~CInstance() = default;

    [[nodiscard]] const vk::raii::Instance& operator*() const noexcept { return m_handle; }
    [[nodiscard]] const vk::raii::Instance* operator->() const noexcept { return &m_handle; }

    [[nodiscard]] bool IsExtensionEnabled(const std::string_view name) const { return m_enabledExtensions.contains(name); }

private:
    [[nodiscard]] std::vector<const char*> SetupExtensions(
        CProfile profile,
        const vk::raii::Context& context,
        std::span<const char* const> requiredExtensions,
        const std::vector<const char*>& enabledLayers
    );

    vk::raii::Instance m_handle { nullptr };

    StringUnorderedSet m_enabledExtensions;

#ifdef DEBUG
    vk::raii::DebugUtilsMessengerEXT m_debugUtilsMessenger { nullptr };
#endif
};
}
