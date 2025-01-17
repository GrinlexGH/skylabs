#include "swapchain.hpp"

#include "console.hpp"

namespace Vulkan {
void CSwapchain::Init(const vk::PhysicalDevice /*physicalDevice*/, const vk::Device device,const vk::SurfaceKHR /*surface*/) {
    vk::SwapchainCreateInfoKHR swapchainCreateInfo;

    m_handle = device.createSwapchainKHR(swapchainCreateInfo);
}
}
