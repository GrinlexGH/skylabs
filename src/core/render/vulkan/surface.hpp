#pragma once
#include "context/context.hpp"

namespace Vulkan {
class CSurface
{
public:
    explicit CSurface(std::nullptr_t);
    explicit CSurface(const CContext* context);
    CSurface(const CSurface&) = delete;
    CSurface(CSurface&& other) noexcept;
    CSurface& operator=(const CSurface&) = delete;
    CSurface& operator=(CSurface&& rhs) noexcept;
    ~CSurface();

    [[nodiscard]] auto GetHandle() const -> vk::SurfaceKHR { return m_handle; }

private:
    auto Destroy() -> void;

    vk::SurfaceKHR m_handle = nullptr;

    const CContext* m_context;
};
}
