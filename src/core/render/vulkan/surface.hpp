#pragma once
#include "render_context.hpp"

namespace Vulkan {
class CSurface
{
public:
    explicit CSurface(const CRenderContext* context);
    CSurface(const CSurface&) = delete;
    CSurface(CSurface&& other) noexcept;
    CSurface& operator=(const CSurface&) = delete;
    CSurface& operator=(CSurface&& rhs) noexcept;
    ~CSurface();

    [[nodiscard]] vk::SurfaceKHR GetHandle() const { return m_handle; }

private:
    vk::SurfaceKHR m_handle = VK_NULL_HANDLE;

    const CRenderContext* m_context;

    void Destroy();
};
}
