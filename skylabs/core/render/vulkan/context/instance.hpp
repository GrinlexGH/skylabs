#pragma once
#include <skylabs/core/render/vulkan/context/extensions.hpp>

#include <vulkan/vulkan_raii.hpp>

namespace Vulkan {
class CInstance
{
public:
    CInstance() = delete;
    explicit CInstance(std::nullptr_t) {}
    explicit CInstance(
        const vk::raii::Context& context,
        std::uint32_t apiVersion,
        std::span<Utils::CRequestedExtension> requestedExtensions
    );
    CInstance(CInstance&) = delete;
    CInstance(CInstance&&) = default;
    CInstance& operator=(CInstance&) = delete;
    CInstance& operator=(CInstance&&) = default;
    ~CInstance() = default;

    [[nodiscard]] auto operator*() const noexcept -> const vk::raii::Instance& { return m_handle; }
    [[nodiscard]] auto operator->() const noexcept -> const vk::raii::Instance* { return &m_handle; }

    [[nodiscard]] auto ApiVersion() const noexcept -> std::uint32_t { return m_apiVersion; }
    [[nodiscard]] auto IsExtensionEnabled(const std::string_view name) const -> bool { return std::ranges::contains(m_activeExtensions, name); }

private:
    vk::raii::Instance m_handle { nullptr };

    std::vector<std::string> m_activeExtensions;
    std::uint32_t m_apiVersion = 0;

#ifdef DEBUG
    vk::raii::DebugUtilsMessengerEXT m_debugUtilsMessenger { nullptr };
#endif
};
}
