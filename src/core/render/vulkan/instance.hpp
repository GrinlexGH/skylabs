#pragma once
#include "vulkan.hpp"
#include "vulkan_window.hpp"
#include "extensions/debug_utils.hpp"

#include <unordered_map>

namespace Vulkan {
class CInstance
{
public:
    struct Config
    {
        const char* gameName { "Game" };
        const char* engineName { "Skylabs" };
        const std::uint32_t m_gameVersion = 0; // todo: version defines in cmake
        const std::uint32_t m_engineBuild = 0;
    };

    explicit CInstance(
        const Config& config,
        const std::unordered_map<const char*, bool>& extensions = {},
        const std::vector<const char*>& layers = {}
    );
    CInstance(const CInstance&) = delete;
    CInstance(CInstance&&) noexcept = default;
    CInstance& operator=(const CInstance&) = delete;
    CInstance& operator=(CInstance&&) noexcept = default;
    ~CInstance();

    [[nodiscard]] vk::Instance GetHandle() const { return m_handle; }

    explicit operator vk::Instance() const { return m_handle; }

private:
    vk::Instance m_handle;

    std::unique_ptr<CDebugUtils> m_debugUtils;
};
}
