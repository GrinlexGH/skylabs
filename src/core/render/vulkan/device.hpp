#pragma once
#include "instance.hpp"

namespace Vulkan {
struct CQueue
{
    vk::Queue m_handle = VK_NULL_HANDLE;
    std::uint32_t m_familyIndex = std::numeric_limits<std::uint32_t>::max();
};

class CDevice
{
public:
    explicit CDevice(
        const CInstance& instance,
        const CPhysicalDevice& physicalDevice,
        const IVulkanWindow* window,
        const std::unordered_map<const char*, bool>& extensions
    );
    CDevice(const CDevice&) = delete;
    CDevice(CDevice&&) = delete;
    CDevice& operator=(const CDevice&) = delete;
    CDevice& operator=(CDevice&&) = delete;
    ~CDevice();

    [[nodiscard]] const CQueue& GetGraphicsQueue() const { return m_graphicsQueue; }
    [[nodiscard]] const CQueue& GetPresentQueue() const { return m_presentQueue; }
    [[nodiscard]] const CQueue& GetTransferQueue() const { return m_transferQueue; }
    [[nodiscard]] const CQueue& GetComputeQueue() const { return m_computeQueue; }

    [[nodiscard]] vk::Device GetHandle() const { return m_handle; }

private:
    vk::Device m_handle = VK_NULL_HANDLE;

    CQueue m_graphicsQueue;
    CQueue m_presentQueue;
    CQueue m_transferQueue;
    CQueue m_computeQueue;
};
}
