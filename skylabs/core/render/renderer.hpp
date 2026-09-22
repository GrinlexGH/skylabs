#pragma once
#include <glm/glm.hpp>

namespace sk::render {
class IRenderer {
public:
    virtual ~IRenderer() = default;

    virtual void Draw(glm::mat4 view, float fov, float deltaTime) = 0;
};
}
