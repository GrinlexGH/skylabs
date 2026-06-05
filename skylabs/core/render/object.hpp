#pragma once
#include <skylabs/core/pch.hpp>

class CObject {
public:
    std::optional<std::uint32_t> meshIndex = std::nullopt;
    std::optional<std::uint32_t> textureIndex = std::nullopt;
    glm::mat4 model { 1 };
};
