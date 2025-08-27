#pragma once
#include "render_context.hpp"

namespace Vulkan {
class CSurface
{
public:
    explicit CSurface(std::nullptr_t);
    explicit CSurface(const CRenderContext* context);
    CSurface(const CSurface&) = delete;
    CSurface(CSurface&& other) noexcept;
    CSurface& operator=(const CSurface&) = delete;
    CSurface& operator=(CSurface&& rhs) noexcept;
    ~CSurface();

    [[nodiscard]] auto GetHandle() const -> vk::SurfaceKHR { return m_handle; }

private:
    vk::SurfaceKHR m_handle = nullptr;

    const CRenderContext* m_context;

    auto Destroy() -> void;
};
}
