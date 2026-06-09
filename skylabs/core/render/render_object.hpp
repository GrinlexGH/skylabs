#pragma once
#include <skylabs/core/pch.hpp>

class CRenderObject {
public:
    std::uint32_t meshId = 0;
    std::uint32_t colorId = 0;
    glm::mat4 model { 1 };
};
