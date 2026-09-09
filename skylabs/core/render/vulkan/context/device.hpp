#pragma once
#include <skylabs/core/render/vulkan/context/physical_device.hpp>

namespace Vulkan {
class CQueue {
public:
    explicit CQueue(std::nullptr_t) { }
    explicit CQueue(const vk::raii::Device& device, const vk::Queue& queue,
                    const std::uint32_t familyIndex)
        : m_handle(device, queue), m_familyIndex(familyIndex) { }
    CQueue(CQueue&) = delete;
    CQueue(CQueue&&) = default;
    CQueue& operator=(CQueue&) = delete;
    CQueue& operator=(CQueue&&) = default;
    ~CQueue() = default;

    [[nodiscard]] const vk::raii::Queue& operator*() const noexcept { return m_handle; }
    [[nodiscard]] const vk::raii::Queue* operator->() const noexcept { return &m_handle; }

    [[nodiscard]] std::uint32_t FamilyIndex() const noexcept { return m_familyIndex; }

private:
    vk::raii::Queue m_handle { nullptr };
    std::uint32_t m_familyIndex = 0;
};

struct DeviceCaps {
    bool maintenance5 = false;
    bool samplerAnisotropy = false;
};

class CDevice {
public:
    explicit CDevice(std::nullptr_t) { }
    explicit CDevice(vk::raii::Device&& device, CPhysicalDevice&& physicalDevice,
                     std::vector<std::string>&& enabledExtensions, const DeviceCaps& caps,
                     CQueue&& graphicsQueue, CQueue&& presentQueue, CQueue&& computeQueue)
        : m_handle(std::move(device)),
          m_physicalDevice(std::move(physicalDevice)),
          m_enabledExtensions(std::move(enabledExtensions)),
          m_caps(caps),
          m_graphicsQueue(std::move(graphicsQueue)),
          m_presentQueue(std::move(presentQueue)),
          m_computeQueue(std::move(computeQueue)) { }
    CDevice(CDevice&) = delete;
    CDevice(CDevice&&) = default;
    CDevice& operator=(CDevice&) = delete;
    CDevice& operator=(CDevice&&) = default;
    ~CDevice() = default;

    [[nodiscard]] const vk::raii::Device& operator*() const noexcept { return m_handle; }
    [[nodiscard]] const vk::raii::Device* operator->() const noexcept { return &m_handle; }

    [[nodiscard]] const CPhysicalDevice& PhysicalDevice() const noexcept { return m_physicalDevice; }

    [[nodiscard]] const CQueue& GraphicsQueue() const noexcept { return m_graphicsQueue; }
    [[nodiscard]] const CQueue& PresentQueue() const noexcept { return m_presentQueue; }
    [[nodiscard]] const CQueue& ComputeQueue() const noexcept { return m_computeQueue; }

    [[nodiscard]] DeviceCaps Caps() const noexcept { return m_caps; }
    [[nodiscard]] bool IsExtensionEnabled(const std::string_view name) const {
        return std::ranges::contains(m_enabledExtensions, name);
    }

private:
    vk::raii::Device m_handle { nullptr };
    CPhysicalDevice m_physicalDevice { nullptr };

    std::vector<std::string> m_enabledExtensions;
    DeviceCaps m_caps;

    CQueue m_graphicsQueue { nullptr };
    CQueue m_presentQueue { nullptr };
    CQueue m_computeQueue { nullptr };
};
}
