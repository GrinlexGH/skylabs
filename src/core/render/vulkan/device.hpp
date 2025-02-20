#pragma once
#include "vulkan.hpp"
#include "queue_families.hpp"
#include "physical_device.hpp"
#include "queues.hpp"
#include "allocator.hpp"
#include "swapchain.hpp"

namespace Vulkan {
class CDevice
{
public:
    explicit CDevice(const vk::Instance& instance, const IVulkanWindow* window);
    CDevice(const CDevice&) = default;
    CDevice(CDevice&&) = default;
    CDevice& operator=(const CDevice&) = default;
    CDevice& operator=(CDevice&&) = default;
    ~CDevice();

    [[nodiscard]] vk::Device GetHandle() const { return m_handle; }

private:
    void Create(const std::vector<const char*>& requiredExtensions);

    CAllocator m_allocator;
    CSwapchain m_swapchain;
    CQueues m_queues;
    CQueueFamilies m_queueFamilies;
    CPhysicalDevice m_physicalDevice;
    vk::Device m_handle;
};
}
