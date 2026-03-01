#pragma once
#include <skylabs/core/render/vulkan/context/instance.hpp>
#include <skylabs/core/render/vulkan/context/device.hpp>
#include <skylabs/core/render/vulkan/context/allocator.hpp>
#include <skylabs/core/render/vulkan/context/profile.hpp>

#include <expected>

namespace Vulkan {
//====================
// Context stores vulkan globals needed to create vulkan objects
class CContext
{
public:
    CContext() = delete;
    explicit CContext(std::nullptr_t) {}
    explicit CContext(const IWindow* window);
    CContext(CContext&) = delete;
    CContext(CContext&&) = default;
    CContext& operator=(CContext&) = delete;
    CContext& operator=(CContext&&) = default;
    ~CContext() = default;

    [[nodiscard]] const IWindow* Window() const noexcept { return m_window; }
    [[nodiscard]] const CInstance& Instance() const noexcept { return m_instance; }
    [[nodiscard]] const CPhysicalDevice& PhysicalDevice() const noexcept { return m_physicalDevice; }
    [[nodiscard]] const CDevice& Device() const noexcept { return m_device; }
    [[nodiscard]] const CAllocator& Allocator() const noexcept { return m_allocator; }

    [[nodiscard]] std::uint32_t ApiVersion() const noexcept { return m_apiVersion; }

private:
    [[nodiscard]] std::expected<CPhysicalDevice, const char*> SelectPhysicalDevice();
    [[nodiscard]] int RatePhysicalDevice(const CPhysicalDevice& physicalDevice) const;

    const IWindow* m_window = nullptr;
    CProfile m_profile { nullptr };
    CInstance m_instance { nullptr };
    CPhysicalDevice m_physicalDevice { nullptr };
    CDevice m_device { nullptr };
    CAllocator m_allocator { nullptr };

    std::uint32_t m_apiVersion = 0;
};
}
