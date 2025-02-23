#pragma once
#include "vulkan.hpp"
#include "vulkan_window.hpp"
#include "extensions/debug_utils.hpp"
#include "extensions/extensions.hpp"

#include <unordered_map>

namespace Vulkan {
class CInstance
{
public:
    struct Config
    {
        const char* m_gameName { "Game" };
        const char* m_engineName { "Skylabs" };
        std::uint32_t m_gameVersion = 0; // todo: version defines in cmake
        std::uint32_t m_engineBuild = 0;
    };

    explicit CInstance(
        const Config& config,
        const std::unordered_map<const char*, bool>& extensions,
        const std::vector<const char*>& layers = {}
    );
    CInstance(const CInstance&) = delete;
    CInstance(CInstance&&) noexcept = default;
    CInstance& operator=(const CInstance&) = delete;
    CInstance& operator=(CInstance&&) noexcept = default;
    ~CInstance();

    [[nodiscard]] vk::Instance GetHandle() const { return m_handle; }
    [[nodiscard]] std::uint32_t GetApiVersion() const { return m_apiVersion; }
    [[nodiscard]] bool IsExtensionEnabled(const std::string_view name) const { return HasExtension(m_enabledExtensions, name); }

    explicit operator vk::Instance() const { return m_handle; }

private:
    vk::Instance m_handle;

    std::uint32_t m_apiVersion;
    std::vector<const char*> m_enabledExtensions;

    std::unique_ptr<CDebugUtils> m_debugUtils;
};
}
