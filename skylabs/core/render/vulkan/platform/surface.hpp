#pragma once
#include <skylabs/core/pch.hpp>
#include <skylabs/core/render/vulkan/context/instance.hpp>
#include <skylabs/core/window.hpp>

namespace Vulkan {
class CSurface
{
public:
    explicit CSurface(std::nullptr_t) {}
    explicit CSurface(const CInstance& instance, const IWindow* window);
    CSurface(const CSurface&) = delete;
    CSurface(CSurface&& other) noexcept = default;
    CSurface& operator=(const CSurface&) = delete;
    CSurface& operator=(CSurface&& rhs) noexcept = default;
    ~CSurface() = default;

    [[nodiscard]] const vk::raii::SurfaceKHR& operator*() const noexcept { return m_handle; }

private:
    vk::raii::SurfaceKHR m_handle { nullptr };
};
}
