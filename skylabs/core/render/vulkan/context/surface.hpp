#pragma once
#include "skylabs/base/vulkan/os_connector.hpp"
#include "skylabs/core/render/vulkan/context/instance.hpp"

namespace sk::render::vulkan {
class Surface {
public:
    explicit Surface(std::nullptr_t) { }
    explicit Surface(const Instance& instance, const sk::vulkan::IOSConnector* osConnector)
        : m_handle(*instance, osConnector->CreateSurface(*instance)) { }
    Surface(const Surface&) = delete;
    Surface(Surface&& other) noexcept = default;
    Surface& operator=(const Surface&) = delete;
    Surface& operator=(Surface&& rhs) noexcept = default;
    ~Surface() = default;

    [[nodiscard]] const vk::raii::SurfaceKHR& operator*() const noexcept { return m_handle; }

private:
    vk::raii::SurfaceKHR m_handle { nullptr };
};
}
