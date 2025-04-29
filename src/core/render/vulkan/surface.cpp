#include "surface.hpp"

namespace Vulkan {
CSurface::CSurface(const CRenderContext* context) : m_context(context) {
    m_handle = m_context->Window()->CreateSurface(m_context->Instance()->GetHandle());
}

CSurface::~CSurface() {
    if (!m_handle) {
        return;
    }

    m_context->Window()->DestroySurface(m_context->Instance()->GetHandle(), m_handle);
}
}
