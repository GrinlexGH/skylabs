#pragma once
#include <skylabs/core/render/vulkan/context/context.hpp>

namespace Vulkan {
class CSurface
{
public:
    explicit CSurface(std::nullptr_t) {}
    explicit CSurface(const CContext& context);
    CSurface(const CSurface&) = delete;
    CSurface(CSurface&& other) noexcept = default;
    CSurface& operator=(const CSurface&) = delete;
    CSurface& operator=(CSurface&& rhs) noexcept = default;
    ~CSurface();

    [[nodiscard]] const vk::raii::SurfaceKHR& operator*() const noexcept { return m_handle; }

private:
    const CContext* m_context = nullptr;

    vk::raii::SurfaceKHR m_handle { nullptr };
};
}
