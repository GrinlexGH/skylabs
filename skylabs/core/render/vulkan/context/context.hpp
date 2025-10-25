#pragma once
#include <skylabs/core/render/vulkan/context/device.hpp>
#include <skylabs/core/render/vulkan/context/instance.hpp>
#include <skylabs/core/render/vulkan/context/physical_device.hpp>
#include <skylabs/core/render/vulkan/context/allocator.hpp>

namespace Vulkan {
//====================
// Context stores vulkan globals needed to create vulkan objects
//====================
class CContext
{
public:
    explicit CContext(std::nullptr_t) {}
    explicit CContext(const IWindow* window);
    CContext(const CContext&) = delete;
    CContext(CContext&&) noexcept = default;
    CContext& operator=(const CContext&) = delete;
    CContext& operator=(CContext&&) noexcept = default;
    ~CContext() = default;

    [[nodiscard]] auto GetWindow() const -> const IWindow* { return m_window; }
    [[nodiscard]] auto GetInstance() const -> const CInstance& { return m_instance; }
    [[nodiscard]] auto GetDevice() const -> const CDevice& { return m_device; }
    [[nodiscard]] auto GetPhysicalDevice() const -> const CPhysicalDevice* { return m_selectedPhysicalDevice; }
    [[nodiscard]] auto GetAllocator() const -> vma::Allocator { return *m_allocator; }

private:
    auto CreateInstance() -> void;
    auto SelectPhysicalDevice() -> void;
    auto CreateLogicalDevice() -> void;
    auto CreateAllocator() -> void;

    [[nodiscard]] auto IsDeviceSuitable(const CPhysicalDevice& physicalDevice) const -> bool;
    [[nodiscard]] auto GetSuitablePhysicalDevice() -> CPhysicalDevice*;

    CInstance m_instance { nullptr };
    CDevice m_device { nullptr };
    CPhysicalDevice* m_selectedPhysicalDevice = nullptr;
    CAllocator m_allocator { nullptr };
    const IWindow* m_window = nullptr;
};
}
