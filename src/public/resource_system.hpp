#pragma once
#include "dll_export.hpp"

#include <vector>
#include <string_view>
#include <cstdint>

namespace ResourceSystem {
//====================
// Defines the relative file search directory
//====================
enum class ResourceType : std::int8_t
{
    eShader = 0,    // shaders/
};

[[nodiscard]] PUBLIC_CLASS std::vector<char> LoadBinary(ResourceType type, const std::string_view relativePath);

[[nodiscard]] inline std::vector<char> LoadShader(const std::string_view relativePath) { return LoadBinary(ResourceType::eShader, relativePath); }
}
