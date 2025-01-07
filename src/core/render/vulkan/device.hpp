#pragma once
#include "vulkan.hpp"
#include "queue_families.hpp"

namespace Vulkan
{
class CDevice
{
public:
    ~CDevice();

    void Create(
        vk::Instance instance,
        vk::PhysicalDevice physicalDevice,
        const std::vector<const char*>& requiredExtensions,
        const IVulkanWindow* window
    );
    [[nodiscard]] vk::Device GetHandle() const { return m_handle; }

private:
    CQueueFamilies m_queueFamilies;

    vk::Device m_handle;
};
}
