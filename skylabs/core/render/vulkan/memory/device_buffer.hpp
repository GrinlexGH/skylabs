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
        vk::BufferUsageFlags usage
    );
    CDeviceBuffer(const CDeviceBuffer&) = delete;
    CDeviceBuffer(CDeviceBuffer&&) noexcept = default;
    CDeviceBuffer& operator=(const CDeviceBuffer&) = delete;
    CDeviceBuffer& operator=(CDeviceBuffer&&) noexcept = default;
    ~CDeviceBuffer() = default;

    [[nodiscard]] auto operator*() const noexcept -> const vma::raii::Buffer& { return m_handle; }
    [[nodiscard]] auto operator->() const noexcept -> const vma::raii::Buffer* { return &m_handle; }

    [[nodiscard]] auto Size() const noexcept -> vk::DeviceSize { return m_size; }
    [[nodiscard]] auto Usage() const noexcept -> vk::BufferUsageFlags { return m_usage; }

private:
    vma::raii::Buffer m_handle { nullptr };

    vk::DeviceSize m_size = 0;
    vk::BufferUsageFlags m_usage;
};
}
