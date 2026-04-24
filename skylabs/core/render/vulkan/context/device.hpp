#pragma once
#include <skylabs/public/window.hpp>
#include <skylabs/core/render/vulkan/context/instance.hpp>
#include <skylabs/core/render/vulkan/context/physical_device.hpp>
#include <skylabs/core/render/vulkan/context/queue.hpp>

namespace Vulkan {
struct DeviceCaps
{
    bool m_maintenance5 = false;
    bool m_samplerAnisotropy = false;
};

class CDevice
{
public:
    explicit CDevice(std::nullptr_t) {}
    explicit CDevice(const CInstance& instance, CPhysicalDevice& physicalDevice);
    CDevice(CDevice&) = delete;
    CDevice(CDevice&&) = default;
    CDevice& operator=(CDevice&) = delete;
    CDevice& operator=(CDevice&&) = default;
    ~CDevice() = default;

    [[nodiscard]] const vk::raii::Device& operator*() const noexcept { return m_handle; }
    [[nodiscard]] const vk::raii::Device* operator->() const noexcept { return &m_handle; }
    [[nodiscard]] const vkb::Device& VkbDevice() const noexcept { return m_vkbDevice; }
    [[nodiscard]] vkb::Device& VkbDevice() noexcept { return m_vkbDevice; }

    [[nodiscard]] const CQueue& GraphicsQueue() const noexcept { return m_graphicsQueue; }
    [[nodiscard]] const CQueue& PresentQueue() const noexcept { return m_presentQueue; }
    [[nodiscard]] const CQueue& ComputeQueue() const noexcept { return m_computeQueue; }

    [[nodiscard]] bool IsExtensionEnabled(const std::string_view name) const { return m_enabledExtensions.contains(name); }
    [[nodiscard]] DeviceCaps Caps() const noexcept { return m_caps; }

private:
    vk::raii::Device m_handle { nullptr };
    vkb::Device m_vkbDevice;

    boost::container::flat_set<std::string, std::less<>> m_enabledExtensions;
    DeviceCaps m_caps;

    CQueue m_graphicsQueue { nullptr };
    CQueue m_presentQueue { nullptr };
    CQueue m_computeQueue { nullptr };
};
}
