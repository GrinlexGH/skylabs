#pragma once
#include <glm/glm.hpp>

namespace sk::render::vulkan {
class Renderer;

class RenderObject {
public:
    RenderObject(std::nullptr_t) { }
    RenderObject(Renderer* renderer, std::uint32_t id) : m_renderer(renderer), m_id(id) { }

    void SetMatrix(const glm::mat4& matrix);
    void SetColor(std::uint16_t colorId);

    [[nodiscard]] glm::mat4 GetMatrix() const;
    [[nodiscard]] std::uint32_t GetId() const { return m_id; }

private:
    Renderer* m_renderer = nullptr;
    std::uint32_t m_id = 0;
};
}
