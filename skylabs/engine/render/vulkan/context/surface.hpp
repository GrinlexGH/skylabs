#pragma once
#include "skylabs/engine/render/vulkan/context/instance.hpp"
#include "skylabs/engine/render/vulkan/os_adapter.hpp"

namespace sk::render::vulkan {
class Surface {
public:
    explicit Surface(std::nullptr_t) { }
    explicit Surface(const Instance& instance, const IOSAdapter* osAdapter)
        : m_handle(*instance, osAdapter->CreateSurface(*instance)) { }

    [[nodiscard]] const vk::raii::SurfaceKHR& operator*() const noexcept { return m_handle; }

private:
    vk::raii::SurfaceKHR m_handle { nullptr };
};
}
