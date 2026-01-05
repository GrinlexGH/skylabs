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
        std::span<Utils::CRequestedExtension> extensions
    );
    CDevice(const CDevice&) = delete;
    CDevice(CDevice&&) noexcept = default;
    CDevice& operator=(const CDevice&) = delete;
    CDevice& operator=(CDevice&&) noexcept = default;
    ~CDevice() = default;

    [[nodiscard]] auto operator*() const noexcept -> const vk::raii::Device& { return m_handle; }
    [[nodiscard]] auto operator->() const noexcept -> const vk::raii::Device* { return &m_handle; }
    [[nodiscard]] auto Handle() const noexcept -> const vk::raii::Device& { return m_handle; }

    [[nodiscard]] auto IsExtensionEnabled(const std::string_view name) const -> bool { return m_enabledExtensions.contains(name); }

    [[nodiscard]] auto GraphicsQueue() const -> const CQueue& { return m_graphicsQueue; }
    [[nodiscard]] auto PresentQueue() const -> const CQueue& { return m_presentQueue; }
    [[nodiscard]] auto TransferQueue() const -> const CQueue& { return m_transferQueue; }
    [[nodiscard]] auto ComputeQueue() const -> const CQueue& { return m_computeQueue; }

private:
    auto EnableExtensions(
        std::span<Utils::CRequestedExtension> requestedExtensions,
        const CPhysicalDevice& physicalDevice,
        std::uint32_t apiVersion
    ) -> std::vector<const char*>;

    UnorderedStringSet m_enabledExtensions;

    vk::raii::Device m_handle = nullptr;

    CQueue m_graphicsQueue;
    CQueue m_presentQueue;
    CQueue m_transferQueue;
    CQueue m_computeQueue;
};
}
