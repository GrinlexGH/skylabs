#pragma once

#include "window.hpp"
#include "vulkan.hpp"

#include <vector>

class IVulkanWindow : public IWindow
{
public:
    virtual ~IVulkanWindow() = default;

    virtual std::vector<const char*> GetRequiredInstanceExtensions() = 0;
    virtual bool GetPresentationSupport(vk::Instance instance, vk::PhysicalDevice physicalDevice, uint32_t queueFamilyIndex) = 0;
    virtual void GetDrawableSize(int* w, int* h) = 0;
    virtual void CreateSurface(vk::Instance& instance) = 0;
    virtual vk::SurfaceKHR GetSurface() { return m_surface; }

protected:
    vk::Instance const* m_instance = nullptr;
    vk::SurfaceKHR m_surface {};
};

