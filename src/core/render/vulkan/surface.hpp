#pragma once
#include "render_context.hpp"
#include "vulkan_window.hpp"

namespace Vulkan {
class CSurface
{
public:
    explicit CSurface(const std::weak_ptr<CRenderContext>& context);
    CSurface(const CSurface&) = delete;
    CSurface(CSurface&&) = delete;
    CSurface& operator=(const CSurface&) = delete;
    CSurface& operator=(CSurface&&) = delete;
    ~CSurface();

    [[nodiscard]] vk::SurfaceKHR GetHandle() const { return m_handle; }

private:
    vk::SurfaceKHR m_handle = VK_NULL_HANDLE;

    std::weak_ptr<CRenderContext> m_context;
};
}
