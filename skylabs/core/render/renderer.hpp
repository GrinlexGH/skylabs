#pragma once
#include <glm/glm.hpp>

class IRenderer
{
public:
    IRenderer() = default;
    IRenderer(const IRenderer&) = delete;
    IRenderer(IRenderer&&) noexcept = default;
    IRenderer& operator=(const IRenderer&) = delete;
    IRenderer& operator=(IRenderer&&) noexcept = default;
    virtual ~IRenderer() = default;

    virtual void Draw(glm::mat4 view, float fov, float deltaTime) = 0;

    bool m_needSwapchainRecreation = false;
    bool m_needSurfaceRecreation = false;
};
