#pragma once
#include <skylabs/core/render/vulkan/context/context.hpp>

namespace Vulkan {
class CDeviceBuffer
{
public:
    explicit CDeviceBuffer(std::nullptr_t) {}
    CDeviceBuffer(
        const CContext& context,
        vk::DeviceSize size,
        const vk::BufferUsageFlags& usage
    );
    CDeviceBuffer(const CDeviceBuffer&) = delete;
    CDeviceBuffer(CDeviceBuffer&&) noexcept = default;
    CDeviceBuffer& operator=(const CDeviceBuffer&) = delete;
    CDeviceBuffer& operator=(CDeviceBuffer&&) noexcept = default;
    ~CDeviceBuffer() = default;

    [[nodiscard]] auto operator*() const noexcept -> vk::Buffer { return *m_handle; }
    [[nodiscard]] auto operator->() const noexcept -> const vk::Buffer* { return &*m_handle; }
    [[nodiscard]] auto Handle() const noexcept -> vk::Buffer { return *m_handle; }

    auto Clear() -> void;

private:
    vma::UniqueBuffer m_handle;
    vma::UniqueAllocation m_allocation;
};
}
