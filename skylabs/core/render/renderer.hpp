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
};
