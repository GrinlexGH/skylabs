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

    [[nodiscard]] const vma::raii::Buffer& operator*() const noexcept { return m_handle; }
    [[nodiscard]] const vma::raii::Buffer* operator->() const noexcept { return &m_handle; }

    [[nodiscard]] vk::DeviceSize Size() const noexcept { return m_size; }
    [[nodiscard]] vk::BufferUsageFlags Usage() const noexcept { return m_usage; }

private:
    vma::raii::Buffer m_handle { nullptr };

    vk::DeviceSize m_size = 0;
    vk::BufferUsageFlags m_usage;
};
}
