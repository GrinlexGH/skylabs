#pragma once
#include "vulkan.hpp"

namespace Vulkan {
class CSwapchain
{
public:
    [[nodiscard]] vk::SwapchainKHR GetHandle() const { return m_handle; }

    void Init(const vk::PhysicalDevice& physicalDevice, const vk::Device& device, const vk::SurfaceKHR& surface);

private:
    vk::SwapchainKHR m_handle;

    
};
}
