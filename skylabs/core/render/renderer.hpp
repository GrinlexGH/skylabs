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

    virtual auto Draw(glm::mat4, float) -> void = 0;

    auto SetResizedState(bool isResized) -> void { m_isResized = isResized; }
    auto GetResizedState() -> bool { return m_isResized; }

private:
    bool m_isResized = false;
};
