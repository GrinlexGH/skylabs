#include "allocator.hpp"

namespace Vulkan
{
void CAllocator::Create(
    const vk::Instance instance,
    const vk::PhysicalDevice physicalDevice,
    const vk::Device device
) {
    vma::AllocatorCreateInfo allocatorCreateInfo;
    allocatorCreateInfo.flags = vma::AllocatorCreateFlagBits::eExtMemoryBudget;
    allocatorCreateInfo.vulkanApiVersion = vk::ApiVersion13;
    allocatorCreateInfo.physicalDevice = physicalDevice;
    allocatorCreateInfo.device = device;
    allocatorCreateInfo.instance = instance;

    const vma::VulkanFunctions vulkanFunctions = vma::functionsFromDispatcher();
    allocatorCreateInfo.pVulkanFunctions = &vulkanFunctions;

    m_handle = createAllocator(allocatorCreateInfo);
}

CAllocator::~CAllocator() {
    if (m_handle) {
        m_handle.destroy();
    }
}
}
