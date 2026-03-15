#include <skylabs/core/pch.hpp>
#include <skylabs/core/render/vulkan/platform/surface.hpp>

namespace Vulkan {
CSurface::CSurface(const CContext& context) : m_context(&context) {
    const vk::raii::Instance& instance = *context.Instance();
    m_handle = vk::raii::SurfaceKHR { instance, context.Window()->CreateSurface(instance) };
}

CSurface::~CSurface() {
    const vk::Instance instanceHandle = *m_context->Instance();
    vk::SurfaceKHR surface = m_handle.release();
    m_context->Window()->DestroySurface(instanceHandle, surface);
}
}
