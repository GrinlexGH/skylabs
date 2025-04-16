#pragma once
#include "instance.hpp"
#include "queue.hpp"

namespace Vulkan {
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

    [[nodiscard]] CQueue GetGraphicsQueue() const { return *m_graphicsQueue; }
    [[nodiscard]] CQueue GetPresentQueue() const { return *m_presentQueue; }
    [[nodiscard]] CQueue GetTransferQueue() const { return *m_transferQueue; }
    [[nodiscard]] CQueue GetComputeQueue() const { return *m_computeQueue; }

    [[nodiscard]] vk::Device GetHandle() const { return m_handle; }

    explicit operator vk::Device() const { return m_handle; }

private:
    vk::Device m_handle;

    std::unique_ptr<CQueue> m_graphicsQueue;
    std::unique_ptr<CQueue> m_presentQueue;
    std::unique_ptr<CQueue> m_transferQueue;
    std::unique_ptr<CQueue> m_computeQueue;
};
}
