#pragma once
#include <vulkan/vulkan.hpp>

namespace Vulkan::Utils {
enum class ExtensionRequirement : std::uint8_t { Required, Optional };

class CRequestedExtension {
public:
    constexpr explicit CRequestedExtension(
        const std::string_view name,
        const ExtensionRequirement requirement = ExtensionRequirement::Optional,
        const std::uint32_t promotedVersion = std::numeric_limits<std::uint32_t>::max()
    ) noexcept
        : m_name(name), m_requirement(requirement), m_promotedVersion(promotedVersion) {}

    [[nodiscard]] constexpr auto Name() const noexcept -> std::string_view { return m_name; }
    [[nodiscard]] constexpr auto Requirement() const noexcept -> ExtensionRequirement { return m_requirement; }
    [[nodiscard]] constexpr auto PromotedVersion() const noexcept -> std::uint32_t { return m_promotedVersion; }

    constexpr std::strong_ordering operator<=>(const CRequestedExtension& rhs) const noexcept {
        if (const auto& cmp = m_name <=> rhs.m_name; cmp != 0)
            return cmp;
        if (m_requirement == rhs.m_requirement)
            return std::strong_ordering::equal;
        return m_requirement == ExtensionRequirement::Required
            ? std::strong_ordering::less
            : std::strong_ordering::greater;
    }

    constexpr bool operator==(const CRequestedExtension& rhs) const noexcept { return m_name == rhs.m_name; }

private:
    std::string_view m_name;
    ExtensionRequirement m_requirement;
    std::uint32_t m_promotedVersion = std::numeric_limits<std::uint32_t>::max();
};

inline bool HasExtension(const std::vector<const char*>& set, const std::string_view target) {
    return std::ranges::any_of(
        set, [&](const char* extension) { return extension == target; }
    );
}

inline bool HasExtension(const std::vector<vk::ExtensionProperties>& set, const std::string_view target) {
    return std::ranges::any_of(
        set, [&](const vk::ExtensionProperties& extension) { return extension.extensionName == target; }
    );
}

inline bool HasLayer(const std::vector<const char*>& set, const std::string_view target) {
    return std::ranges::any_of(
        set, [&](const char* layer) { return layer == target; }
    );
}

inline bool HasLayer(const std::vector<vk::LayerProperties>& set, const std::string_view target) {
    return std::ranges::any_of(
        set, [&](const vk::LayerProperties& layer) { return layer.layerName == target; }
    );
}

inline void AppendToPNextChain(void*& currentChain, void* newExtension) {
    if (currentChain == nullptr) {
        currentChain = newExtension;
        return;
    }

    auto* current = static_cast<vk::BaseOutStructure*>(currentChain);
    while (current->pNext != nullptr) {
        current = current->pNext;
    }

    current->pNext = static_cast<vk::BaseOutStructure*>(newExtension);
}
}
