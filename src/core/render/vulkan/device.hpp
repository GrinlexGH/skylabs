#pragma once
#include "vulkan.hpp"
#include "queue_families.hpp"
#include "physical_device.hpp"
#include "queues.hpp"
#include "allocator.hpp"

namespace Vulkan
{
class CDevice
{
public:
    CDevice() = default;
    CDevice(const CDevice&) = default;
    CDevice(CDevice&&) = default;
    CDevice& operator=(const CDevice&) = default;
    CDevice& operator=(CDevice&&) = default;
    ~CDevice();

    void Initialize(
        vk::Instance instance,
        const IVulkanWindow* window
    );
    [[nodiscard]] vk::Device GetHandle() const { return m_handle; }

private:
    void Create(const std::vector<const char*>& requiredExtensions);

    CAllocator m_allocator;
    CQueues m_queues;
    CQueueFamilies m_queueFamilies;
    CPhysicalDevice m_physicalDevice;
    vk::Device m_handle;
};
}
