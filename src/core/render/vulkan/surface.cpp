#include "surface.hpp"

namespace Vulkan {
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
        std::swap(m_handle, rhs.m_handle);
        std::swap(m_context, rhs.m_context);
    }
    return *this;
}

CSurface::~CSurface() {
    if (m_handle) {
        const vk::Instance instanceHandle = m_context->GetInstance().GetHandle();
        const IWindow* window = m_context->GetWindow();
        window->DestroySurface(instanceHandle, m_handle);
    }
}
}
