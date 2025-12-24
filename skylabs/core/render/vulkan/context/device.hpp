#pragma once
#include <skylabs/core/render/vulkan/context/instance.hpp>
#include <skylabs/core/render/vulkan/window.hpp>
#include <skylabs/public/string_utils.hpp>

namespace Vulkan {
struct CQueue
{
    vk::raii::Queue m_handle = nullptr;
    std::uint32_t m_familyIndex = std::numeric_limits<std::uint32_t>::max();
};

class CDevice
{
public:
    explicit CDevice(std::nullptr_t) {}
    explicit CDevice(
        const CInstance& instance,
        const CPhysicalDevice& physicalDevice,
        const IWindow* window,
        std::span<RequestedExtension> extensions
    );
    CDevice(const CDevice&) = delete;
    CDevice(CDevice&&) noexcept = default;
    CDevice& operator=(const CDevice&) = delete;
    CDevice& operator=(CDevice&&) noexcept = default;
    ~CDevice() = default;

    auto operator*() const noexcept -> const vk::raii::Device& { return m_handle; }
    [[nodiscard]] auto GetHandle() const -> const vk::raii::Device& { return m_handle; }

    [[nodiscard]] auto IsExtensionEnabled(const std::string_view name) const -> bool { return m_enabledExtensions.contains(name); }

    [[nodiscard]] auto GetGraphicsQueue() const -> const CQueue& { return m_graphicsQueue; }
    [[nodiscard]] auto GetPresentQueue() const -> const CQueue& { return m_presentQueue; }
    [[nodiscard]] auto GetTransferQueue() const -> const CQueue& { return m_transferQueue; }
    [[nodiscard]] auto GetComputeQueue() const -> const CQueue& { return m_computeQueue; }

private:
    auto EnableExtensions(
        std::span<RequestedExtension> requestedExtensions,
        const CPhysicalDevice& physicalDevice
    ) -> std::vector<const char*>;

    UnorderedStringSet m_enabledExtensions;

    vk::raii::Device m_handle = nullptr;

    CQueue m_graphicsQueue;
    CQueue m_presentQueue;
    CQueue m_transferQueue;
    CQueue m_computeQueue;
};
}
