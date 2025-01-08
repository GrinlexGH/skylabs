#pragma once
#include "vulkan.hpp"
#include "queue_families.hpp"
#include "physical_device.hpp"

namespace Vulkan
{
class CDevice
{
public:
    ~CDevice();

    void Initialize(
        vk::Instance instance,
        const IVulkanWindow* window
    );
    [[nodiscard]] vk::Device GetHandle() const { return m_handle; }

private:
    void Create(const std::vector<const char*>& requiredExtensions);

    void CreateAllocator(vk::Instance instance);

    CQueueFamilies m_queueFamilies;
    vma::Allocator m_allocator;
    CPhysicalDevice m_physicalDevice;
    vk::Device m_handle;
};
}
