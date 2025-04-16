#pragma once
#include "device.hpp"
#include "instance.hpp"
#include "physical_device.hpp"
#include "vulkan_window.hpp"

namespace Vulkan {
//====================
// Context stores vulkan globals needed to create vulkan objects
//====================
class CRenderContext
{
public:
    explicit CRenderContext(const IVulkanWindow* window);
    CRenderContext(const CRenderContext&) = delete;
    CRenderContext(CRenderContext&&) = delete;
    CRenderContext& operator=(const CRenderContext&) = delete;
    CRenderContext& operator=(CRenderContext&&) = delete;
    ~CRenderContext() = default;

    [[nodiscard]] const IVulkanWindow* Window() const { return m_window; }
    [[nodiscard]] const CInstance* Instance() const { return m_instance.get(); }
    [[nodiscard]] const CDevice* Device() const { return m_device.get(); }
    [[nodiscard]] const CPhysicalDevice* PhysicalDevice() const { return m_selectedPhysicalDevice; }

    [[nodiscard]] vk::SurfaceKHR CreateSurface() const { return m_window && m_instance ? m_window->CreateSurface(m_instance->GetHandle()) : VK_NULL_HANDLE; }
    void DestroySurface(vk::SurfaceKHR& surface) const { if (m_window && m_instance) { m_window->DestroySurface(m_instance->GetHandle(), surface); } }

private:
    void CreateInstance();
    void SelectPhysicalDevice();
    void CreateLogicalDevice();

    std::unique_ptr<CInstance> m_instance;
    std::unique_ptr<CDevice> m_device;
    CPhysicalDevice* m_selectedPhysicalDevice = nullptr;
    const IVulkanWindow* const m_window = nullptr;
};
}
