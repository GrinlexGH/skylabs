#include "surface.hpp"

namespace Vulkan {
CSurface::CSurface(const CRenderContext* context) : m_context(context) {
    m_handle = m_context->CreateSurface();
}

CSurface::~CSurface() {
    if (!m_handle) {
        return;
    }

    m_context->DestroySurface(m_handle);
}
}
