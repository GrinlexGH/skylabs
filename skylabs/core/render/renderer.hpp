#pragma once
#include <glm/glm.hpp>

class IRenderer
{
public:
    virtual ~IRenderer() = default;

    virtual void Draw(glm::mat4 view, float fov, float deltaTime) = 0;

    bool m_needSwapchainRecreation = false;
    bool m_needSurfaceRecreation = false;
};
