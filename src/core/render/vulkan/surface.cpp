#include "surface.hpp"

namespace Vulkan {
CSurface::CSurface(const CRenderContext* context) : m_context(context) {
    const vk::Instance instance = m_context->GetInstance()->GetHandle();
    const IVulkanWindow* window = m_context->GetWindow();
    m_handle = window->CreateSurface(instance);
}

CSurface::~CSurface() {
    if (!m_handle) {
        return;
    }

    const vk::Instance instance = m_context->GetInstance()->GetHandle();
    const IVulkanWindow* window = m_context->GetWindow();
    window->DestroySurface(instance, m_handle);
}
}
