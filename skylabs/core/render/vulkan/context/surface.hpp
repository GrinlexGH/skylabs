#pragma once
#include <skylabs/core/render/vulkan/context/instance.hpp>
#include <skylabs/public/vulkan/os_connector.hpp>

namespace Vulkan {
class CSurface
{
public:
    explicit CSurface(std::nullptr_t) {}
    explicit CSurface(const CInstance& instance, const IOSConnector* surfaceProvider) :
        m_handle(*instance, surfaceProvider->CreateSurface(*instance)) {}
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
