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

    [[nodiscard]] const IVulkanWindow* GetWindow() const { return m_window; }
    [[nodiscard]] const CInstance* GetInstance() const { return m_instance.get(); }
    [[nodiscard]] const CDevice* GetDevice() const { return m_device.get(); }
    [[nodiscard]] const CPhysicalDevice* GetPhysicalDevice() const { return m_selectedPhysicalDevice; }

private:
    void CreateInstance();
    void SelectPhysicalDevice();
    void CreateLogicalDevice();

    std::unique_ptr<CInstance> m_instance;
    std::unique_ptr<CDevice> m_device;
    CPhysicalDevice* m_selectedPhysicalDevice;
    const IVulkanWindow* const m_window;
};
}
