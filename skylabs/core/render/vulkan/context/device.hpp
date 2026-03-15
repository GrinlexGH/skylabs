#pragma once
#include <skylabs/core/render/vulkan/platform/window.hpp>
#include <skylabs/core/render/vulkan/context/profile.hpp>
#include <skylabs/core/render/vulkan/context/instance.hpp>
#include <skylabs/core/render/vulkan/context/physical_device.hpp>
#include <skylabs/core/render/vulkan/context/queue.hpp>

namespace Vulkan {
class CDevice
{
public:
    explicit CDevice(std::nullptr_t) {}
    explicit CDevice(
        CProfile profile,
        const IWindow* window,
        const CInstance& instance,
        const CPhysicalDevice& physicalDevice
    );
    CDevice(CDevice&) = delete;
    CDevice(CDevice&&) = default;
    CDevice& operator=(CDevice&) = delete;
    CDevice& operator=(CDevice&&) = default;
    ~CDevice() = default;

    [[nodiscard]] const vk::raii::Device& operator*() const noexcept { return m_handle; }
    [[nodiscard]] const vk::raii::Device* operator->() const noexcept { return &m_handle; }

    [[nodiscard]] const CQueue& GraphicsQueue() const noexcept { return m_graphicsQueue; }
    [[nodiscard]] const CQueue& PresentQueue() const noexcept { return m_presentQueue; }
    [[nodiscard]] const CQueue& ComputeQueue() const noexcept { return m_computeQueue; }

    [[nodiscard]] bool IsExtensionEnabled(const std::string_view name) const { return std::ranges::contains(m_enabledExtensions, name); }

private:
    [[nodiscard]] std::vector<const char*> SetupExtensions(CProfile profile, const CPhysicalDevice& gpu);

    vk::raii::Device m_handle { nullptr };

    StringUnorderedSet m_enabledExtensions;

    CQueue m_graphicsQueue { nullptr };
    CQueue m_presentQueue { nullptr };
    CQueue m_computeQueue { nullptr };
};
}
