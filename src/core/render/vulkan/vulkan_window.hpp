#pragma once
#include "../../window.hpp"

#include "vulkan.hpp"

#include <vector>

class IVulkanWindow : public IWindow
{
public:
    IVulkanWindow() = default;
    IVulkanWindow(const IVulkanWindow&) = default;
    IVulkanWindow(IVulkanWindow&&) = default;
    IVulkanWindow& operator=(const IVulkanWindow&) = default;
    IVulkanWindow& operator=(IVulkanWindow&&) = default;
    ~IVulkanWindow() override = default;

    [[nodiscard]] virtual std::vector<const char*> GetRequiredInstanceExtensions() const = 0;
    [[nodiscard]] virtual bool CheckQueuePresentSupport(vk::Instance instance, vk::PhysicalDevice physicalDevice, uint32_t queueFamilyIndex) const = 0;

    virtual void CreateSurface(vk::Instance instance) = 0;
    virtual void DestroySurface(vk::Instance instance) = 0;
    [[nodiscard]] vk::SurfaceKHR GetSurface() const { return m_surface; }

protected:
    vk::SurfaceKHR m_surface;
};
