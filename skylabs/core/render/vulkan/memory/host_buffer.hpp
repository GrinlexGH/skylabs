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

    [[nodiscard]] auto operator*() const noexcept -> const vma::raii::Buffer& { return m_handle; }
    [[nodiscard]] auto operator->() const noexcept -> const vma::raii::Buffer* { return &m_handle; }

    [[nodiscard]] auto Data() const noexcept -> void* { return m_data; }
    [[nodiscard]] auto Size() const noexcept -> std::size_t { return m_size; }
    [[nodiscard]] auto Span() const -> std::span<std::byte> { return { static_cast<std::byte*>(m_data), static_cast<std::size_t>(m_size) }; }
    [[nodiscard]] auto Usage() const noexcept -> vk::BufferUsageFlags { return m_usage; }

private:
    vma::raii::Buffer m_handle { nullptr };

    void* m_data = nullptr;
    vk::DeviceSize m_size = 0;
    vk::BufferUsageFlags m_usage;
};
}
