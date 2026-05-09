#pragma once
#include <skylabs/core/render/vulkan/context/instance.hpp>
#include <skylabs/core/render/vulkan/context/device.hpp>
#include <skylabs/core/render/vulkan/context/allocator.hpp>
#include <skylabs/core/render/vulkan/platform/surface.hpp>

namespace Vulkan {
class CDeviceContext
{
public:
    CDeviceContext() = delete;
    explicit CDeviceContext(std::nullptr_t) {}
    explicit CDeviceContext(const IWindow* window);
    CDeviceContext(CDeviceContext&) = delete;
    CDeviceContext(CDeviceContext&&) = default;
    CDeviceContext& operator=(CDeviceContext&) = delete;
    CDeviceContext& operator=(CDeviceContext&&) = default;
    ~CDeviceContext() = default;

    [[nodiscard]] const IWindow* Window() const noexcept { return m_window; }
    [[nodiscard]] const CInstance& Instance() const noexcept { return m_instance; }
    [[nodiscard]] const CSurface& Surface() const noexcept { return m_surface; }
    [[nodiscard]] const CPhysicalDevice& PhysicalDevice() const noexcept { return m_physicalDevice; }
    [[nodiscard]] const CDevice& Device() const noexcept { return m_device; }
    [[nodiscard]] const CAllocator& Allocator() const noexcept { return m_allocator; }

    void RepairSurface();

private:
    const IWindow* m_window = nullptr;
    CInstance m_instance { nullptr };
    CSurface m_surface { nullptr };
    CPhysicalDevice m_physicalDevice { nullptr };
    CDevice m_device { nullptr };
    CAllocator m_allocator { nullptr };
};
}
