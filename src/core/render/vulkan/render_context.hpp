#pragma once
#include "device.hpp"
#include "instance.hpp"
#include "physical_device.hpp"

namespace Vulkan {
//====================
// Context stores vulkan globals needed to create vulkan objects
//====================
class CRenderContext
{
public:
    explicit CRenderContext(const IWindow* window);
    CRenderContext(const CRenderContext&) = delete;
    CRenderContext(CRenderContext&&) noexcept = default;
    CRenderContext& operator=(const CRenderContext&) = delete;
    CRenderContext& operator=(CRenderContext&&) noexcept = default;
    ~CRenderContext() = default;

    [[nodiscard]] const IWindow* GetWindow() const { return m_window; }
    [[nodiscard]] const CInstance* GetInstance() const { return m_instance.get(); }
    [[nodiscard]] const CDevice* GetDevice() const { return m_device.get(); }
    [[nodiscard]] const CPhysicalDevice* GetPhysicalDevice() const { return m_selectedPhysicalDevice; }

private:
    void CreateInstance();
    void SelectPhysicalDevice();
    void CreateLogicalDevice();

    [[nodiscard]] bool IsDeviceSuitable(const CPhysicalDevice* physicalDevice) const;
    [[nodiscard]] CPhysicalDevice* GetSuitablePhysicalDevice() const;

    std::unique_ptr<CInstance> m_instance;
    std::unique_ptr<CDevice> m_device;
    CPhysicalDevice* m_selectedPhysicalDevice;
    const IWindow* m_window;
};
}
