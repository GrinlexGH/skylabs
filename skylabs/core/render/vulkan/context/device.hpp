#pragma once
#include "skylabs/core/render/vulkan/context/physical_device.hpp"

namespace sk::render::vulkan {
class Queue {
public:
    explicit Queue(std::nullptr_t) { }
    explicit Queue(const vk::raii::Device& device, const vk::Queue& queue,
                   const std::uint32_t familyIndex)
        : m_handle(device, queue), m_familyIndex(familyIndex) { }
    Queue(Queue&) = delete;
    Queue(Queue&&) = default;
    Queue& operator=(Queue&) = delete;
    Queue& operator=(Queue&&) = default;
    ~Queue() = default;

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

class Device {
public:
    explicit Device(std::nullptr_t) { }
    explicit Device(vk::raii::Device&& device, PhysicalDevice&& physicalDevice,
                    std::vector<std::string>&& enabledExtensions, const DeviceCaps& caps,
                    Queue&& graphicsQueue, Queue&& presentQueue, Queue&& computeQueue)
        : m_handle(std::move(device)),
          m_physicalDevice(std::move(physicalDevice)),
          m_enabledExtensions(std::move(enabledExtensions)),
          m_caps(caps),
          m_graphicsQueue(std::move(graphicsQueue)),
          m_presentQueue(std::move(presentQueue)),
          m_computeQueue(std::move(computeQueue)) { }
    Device(Device&) = delete;
    Device(Device&&) = default;
    Device& operator=(Device&) = delete;
    Device& operator=(Device&&) = default;
    ~Device() = default;

    [[nodiscard]] const vk::raii::Device& operator*() const noexcept { return m_handle; }
    [[nodiscard]] const vk::raii::Device* operator->() const noexcept { return &m_handle; }

    [[nodiscard]] const PhysicalDevice& GetPhysicalDevice() const noexcept { return m_physicalDevice; }

    [[nodiscard]] const Queue& GraphicsQueue() const noexcept { return m_graphicsQueue; }
    [[nodiscard]] const Queue& PresentQueue() const noexcept { return m_presentQueue; }
    [[nodiscard]] const Queue& ComputeQueue() const noexcept { return m_computeQueue; }

    [[nodiscard]] DeviceCaps Caps() const noexcept { return m_caps; }
    [[nodiscard]] bool IsExtensionEnabled(const std::string_view name) const {
        return std::ranges::contains(m_enabledExtensions, name);
    }

private:
    vk::raii::Device m_handle { nullptr };
    PhysicalDevice m_physicalDevice { nullptr };

    std::vector<std::string> m_enabledExtensions;
    DeviceCaps m_caps;

    Queue m_graphicsQueue { nullptr };
    Queue m_presentQueue { nullptr };
    Queue m_computeQueue { nullptr };
};
}
