#include "surface.hpp"

namespace Vulkan {
CSurface::CSurface(std::nullptr_t) {}

CSurface::CSurface(const CContext* context) : m_context(context) {
    const vk::Instance instanceHandle = m_context->GetInstance().GetHandle();
    const IWindow* window = m_context->GetWindow();
    m_handle = window->CreateSurface(instanceHandle);
}

CSurface::CSurface(CSurface&& other) noexcept :
    m_handle(std::exchange(other.m_handle, nullptr)),
    m_context(std::exchange(other.m_context, nullptr)) {}

CSurface& CSurface::operator=(CSurface&& rhs) noexcept {
    if (this != &rhs) {
        if (m_handle) { Destroy(); }
        m_handle = std::exchange(rhs.m_handle, nullptr);
        m_context = std::exchange(rhs.m_context, nullptr);
    }
    return *this;
}

CSurface::~CSurface() {
    if (m_handle) { Destroy(); }
}

auto CSurface::Destroy() -> void {
    const vk::Instance instanceHandle = m_context->GetInstance().GetHandle();
    const IWindow* window = m_context->GetWindow();
    window->DestroySurface(instanceHandle, m_handle);
}
}
