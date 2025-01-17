#pragma once
#include "vulkan.hpp"

namespace Vulkan {
class CSwapchain
{
public:
    [[nodiscard]] vk::SwapchainKHR GetHandle() const { return m_handle; }

    void Init(vk::PhysicalDevice physicalDevice, vk::Device device, vk::SurfaceKHR surface);

private:
    vk::SwapchainKHR m_handle;
};
}
