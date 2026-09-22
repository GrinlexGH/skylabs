#pragma once
#include <skylabs/core/pch.hpp>

namespace sk {
class RenderObject {
public:
    std::uint32_t meshId = 0;
    std::uint32_t colorId = 0;
    glm::mat4 model { 1 };
};
}
