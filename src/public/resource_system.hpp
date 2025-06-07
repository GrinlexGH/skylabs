#pragma once
#include "dll_export.hpp"

#include <vector>
#include <cstdint>

namespace ResourceSystem {
//====================
// Defines the relative file search directory
//====================
enum class ResourceType : std::int8_t
{
    eShader = 0,    // shaders/
};

[[nodiscard]] PUBLIC_CLASS std::vector<char> LoadBinary(ResourceType type, const char* relativePath);

[[nodiscard]] inline std::vector<char> LoadShader(const char* relativePath) { return LoadBinary(ResourceType::eShader, relativePath); }
}
