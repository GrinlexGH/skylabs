#include <skylabs/core/pch.hpp>
#include <skylabs/core/render/vulkan/platform/surface.hpp>

namespace Vulkan {
CSurface::CSurface(const CInstance& instance, const IWindow* window) {
    m_handle = vk::raii::SurfaceKHR { *instance, window->CreateSurface(*instance) };
}
}
