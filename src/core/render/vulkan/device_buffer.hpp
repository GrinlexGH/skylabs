#pragma once
#include "context/context.hpp"

namespace Vulkan {
class CDeviceBuffer
{
public:
    explicit CDeviceBuffer(std::nullptr_t) {}
    CDeviceBuffer(
        const CContext* context,
        vk::DeviceSize size,
        const vk::BufferUsageFlags& usage
    );
    CDeviceBuffer(const CDeviceBuffer&) = delete;
    CDeviceBuffer(CDeviceBuffer&&) noexcept = default;
    CDeviceBuffer& operator=(const CDeviceBuffer&) = delete;
    CDeviceBuffer& operator=(CDeviceBuffer&&) noexcept = default;
    ~CDeviceBuffer();

    auto operator*() const noexcept -> vk::Buffer { return *m_handle; }
    [[nodiscard]] auto GetHandle() const -> vk::Buffer { return *m_handle; }

    auto Clear() -> void;

private:
    vma::UniqueBuffer m_handle;
    vma::UniqueAllocation m_allocation;

    const CContext* m_context = nullptr;
};
}
