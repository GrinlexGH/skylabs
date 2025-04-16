#include "surface.hpp"

#include "../renderer.hpp"

namespace Vulkan {
CSurface::CSurface(const std::weak_ptr<CRenderContext>& context) : m_context(context) {
    if (const auto ctx = m_context.lock()) {
        m_handle = ctx->CreateSurface();
    } else {
        throw CRendererInitError("Couldn't create vulkan surface. Render context is expired!");
    }
}

CSurface::~CSurface() {
    if (!m_handle) {
        return;
    }

    if (const auto ctx = m_context.lock()) {
        ctx->DestroySurface(m_handle);
    } else {
        Log::Error("Couldn't properly destroy vulkan surface. Render context is expired!");
    }
}
}
