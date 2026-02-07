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

    virtual void Draw(glm::mat4, float) = 0;

    void SetResized() { m_isResized = true; }
    bool IsResized() const { return m_isResized; }

protected:
    bool m_isResized = false;
};
