#pragma once
#include <skylabs/core/render/vulkan/context/instance.hpp>
#include <skylabs/core/render/vulkan/context/device.hpp>
#include <skylabs/core/render/vulkan/context/allocator.hpp>

namespace Vulkan {
//====================
// Context stores vulkan globals needed to create vulkan objects
//====================
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

    [[nodiscard]] auto Window() const noexcept -> const IWindow* { return m_window; }
    [[nodiscard]] auto Instance() const noexcept -> const CInstance& { return m_instance; }
    [[nodiscard]] auto PhysicalDevice() const noexcept -> const vk::raii::PhysicalDevice& { return m_physicalDevice; }
    [[nodiscard]] auto Device() const noexcept -> const CDevice& { return m_device; }
    [[nodiscard]] auto Allocator() const noexcept -> const CAllocator& { return m_allocator; }

private:
    auto CreateInstance() -> void;
    [[nodiscard]] auto SelectPhysicalDevice(const vk::raii::PhysicalDevices& physicalDevices) const -> CPhysicalDevice;
    [[nodiscard]] auto IsDeviceSuitable(const CPhysicalDevice& physicalDeviceInfo) const -> bool;
    [[nodiscard]] auto RatePhysicalDevice(const CPhysicalDevice& physicalDeviceInfo) const -> int;
    auto CreateDevice() -> void;

    const IWindow* m_window = nullptr;
    vk::raii::Context m_context;
    CInstance m_instance { nullptr };
    vk::raii::PhysicalDevice m_physicalDevice { nullptr };
    CDevice m_device { nullptr };
    CAllocator m_allocator { nullptr };
};
}
