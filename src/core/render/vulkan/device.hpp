#pragma once
#include "instance.hpp"
#include "vulkan_window.hpp"

namespace Vulkan {
struct CQueue
{
    vk::raii::Queue m_handle = VK_NULL_HANDLE;
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
    CDevice(CDevice&&) noexcept = default;
    CDevice& operator=(const CDevice&) = delete;
    CDevice& operator=(CDevice&&) noexcept = default;
    ~CDevice() = default;

    [[nodiscard]] const CQueue& GetGraphicsQueue() const { return m_graphicsQueue; }
    [[nodiscard]] const CQueue& GetPresentQueue() const { return m_presentQueue; }
    [[nodiscard]] const CQueue& GetTransferQueue() const { return m_transferQueue; }
    [[nodiscard]] const CQueue& GetComputeQueue() const { return m_computeQueue; }

    [[nodiscard]] const vk::raii::Device& GetHandle() const { return m_handle; }

private:
    vk::raii::Device m_handle = VK_NULL_HANDLE;

    CQueue m_graphicsQueue;
    CQueue m_presentQueue;
    CQueue m_transferQueue;
    CQueue m_computeQueue;
};
}
