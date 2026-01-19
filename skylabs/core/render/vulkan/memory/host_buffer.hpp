#pragma once
#include <skylabs/core/render/vulkan/context/context.hpp>

namespace Vulkan {
class CHostBuffer
{
public:
    explicit CHostBuffer(std::nullptr_t) {}
    CHostBuffer(
        const CContext& context,
        vk::DeviceSize size,
        vk::BufferUsageFlags usage
    );
    CHostBuffer(const CHostBuffer&) = delete;
    CHostBuffer(CHostBuffer&&) noexcept = default;
    CHostBuffer& operator=(const CHostBuffer&) = delete;
    CHostBuffer& operator=(CHostBuffer&&) noexcept = default;
    ~CHostBuffer();

    [[nodiscard]] const vma::raii::Buffer& operator*() const noexcept { return m_handle; }
    [[nodiscard]] const vma::raii::Buffer* operator->() const noexcept { return &m_handle; }

    [[nodiscard]] void* Data() const noexcept { return m_data; }
    [[nodiscard]] std::size_t Size() const noexcept { return m_size; }
    [[nodiscard]] std::span<std::byte> Span() const { return { static_cast<std::byte*>(m_data), static_cast<std::size_t>(m_size) }; }
    [[nodiscard]] vk::BufferUsageFlags Usage() const noexcept { return m_usage; }

private:
    vma::raii::Buffer m_handle { nullptr };

    void* m_data = nullptr;
    vk::DeviceSize m_size = 0;
    vk::BufferUsageFlags m_usage;
};
}
