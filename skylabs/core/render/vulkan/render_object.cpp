#include <skylabs/core/render/vulkan/render_object.hpp>
#include <skylabs/core/render/vulkan/renderer.hpp>

namespace Vulkan {
void CRenderObject::SetMatrix(const glm::mat4& matrix) {
    m_renderer->GetObjectData(m_id).model = matrix;
}

void CRenderObject::SetColor(std::uint16_t colorId) {
    m_renderer->GetObjectData(m_id).colorId = colorId;
}

glm::mat4 CRenderObject::GetMatrix() const {
    return m_renderer->GetObjectData(m_id).model;
}
}
