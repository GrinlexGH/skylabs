#pragma once
#include "vulkan_window.hpp"
#include "render_context.hpp"

namespace Vulkan {
class CSurface
{
public:
    explicit CSurface(const CRenderContext* context);
    CSurface(const CSurface&) = delete;
    CSurface(CSurface&&) = delete;
    CSurface& operator=(const CSurface&) = delete;
    CSurface& operator=(CSurface&&) = delete;
    ~CSurface();

    [[nodiscard]] vk::SurfaceKHR GetHandle() const { return m_handle; }

private:
    vk::SurfaceKHR m_handle = VK_NULL_HANDLE;

    const CRenderContext* m_context;
};
}
