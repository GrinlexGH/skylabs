#pragma once
#include <skylabs/core/render/vulkan/window.hpp>
#include <skylabs/core/render/vulkan/context/instance.hpp>
#include <skylabs/core/render/vulkan/context/physical_device.hpp>
#include <skylabs/core/render/vulkan/context/queue.hpp>

namespace Vulkan {
class CDevice
{
public:
    explicit CDevice(std::nullptr_t) {}
    explicit CDevice(
        const IWindow* window,
        const CInstance& instance,
        const CPhysicalDevice& physicalDevice
    );
    CDevice(CDevice&) = delete;
    CDevice(CDevice&&) = default;
    CDevice& operator=(CDevice&) = delete;
    CDevice& operator=(CDevice&&) = default;
    ~CDevice() = default;

    [[nodiscard]] auto operator*() const noexcept -> const vk::raii::Device& { return m_handle; }
    [[nodiscard]] auto operator->() const noexcept -> const vk::raii::Device* { return &m_handle; }

    [[nodiscard]] auto GraphicsQueue() const noexcept -> const CQueue& { return m_graphicsQueue; }
    [[nodiscard]] auto PresentQueue() const noexcept -> const CQueue& { return m_presentQueue; }
    [[nodiscard]] auto ComputeQueue() const noexcept -> const CQueue& { return m_computeQueue; }

    [[nodiscard]] auto IsExtensionEnabled(const std::string_view name) const -> bool { return std::ranges::contains(m_enabledExtensions, name); }

private:
    [[nodiscard]] auto SetupExtensions(const CPhysicalDevice& gpu) -> std::vector<const char*>;

    vk::raii::Device m_handle { nullptr };

    UnorderedStringSet m_enabledExtensions;

    CQueue m_graphicsQueue { nullptr };
    CQueue m_presentQueue { nullptr };
    CQueue m_computeQueue { nullptr };
};
}
