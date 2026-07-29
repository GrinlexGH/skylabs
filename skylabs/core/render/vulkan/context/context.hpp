#pragma once
#include <skylabs/core/render/vulkan/context/allocator.hpp>
#include <skylabs/public/window.hpp>

namespace Vulkan
{
class CContext
{
public:
    CContext() = delete;
    explicit CContext(std::nullptr_t) {}
    explicit CContext(const IWindow* window, const IOSConnector* osConnector);
    CContext(CContext&) = delete;
    CContext(CContext&&) = default;
    CContext& operator=(CContext&) = delete;
    CContext& operator=(CContext&&) = default;
    ~CContext() = default;

    [[nodiscard]] const IWindow* Window() const noexcept { return m_window; }
    [[nodiscard]] const CInstance& Instance() const noexcept { return m_instance; }
    [[nodiscard]] const CSurface& Surface() const noexcept { return m_surface; }
    [[nodiscard]] const CPhysicalDevice& PhysicalDevice() const noexcept { return m_physicalDevice; }
    [[nodiscard]] const CDevice& Device() const noexcept { return m_device; }
    [[nodiscard]] const CAllocator& Allocator() const noexcept { return m_allocator; }

    void RepairSurface();

private:
    const IWindow* m_window = nullptr;
    const IOSConnector* m_osConnector = nullptr;
    CInstance m_instance { nullptr };
    CSurface m_surface { nullptr };
    CPhysicalDevice m_physicalDevice { nullptr };
    CDevice m_device { nullptr };
    CAllocator m_allocator { nullptr };
};
}
