#pragma once
#include <vk_mem_alloc_raii.hpp>

namespace Vulkan {
class CBufferMapping
{
public:
    explicit CBufferMapping(vma::raii::Buffer& buffer);
    CBufferMapping(const CBufferMapping&) = delete;
    CBufferMapping(CBufferMapping&&) noexcept = default;
    CBufferMapping& operator=(const CBufferMapping&) = delete;
    CBufferMapping& operator=(CBufferMapping&&) noexcept = default;
    ~CBufferMapping();

    [[nodiscard]] auto operator*() const noexcept -> void* { return m_data; }
    [[nodiscard]] auto Data() const noexcept -> void* { return m_data; }
    [[nodiscard]] auto Size() const noexcept -> std::size_t { return m_size; }
    [[nodiscard]] auto Span() const -> std::span<std::byte> { return { static_cast<std::byte*>(m_data), static_cast<std::size_t>(m_size) }; }

private:
    vma::raii::Buffer* m_buffer = nullptr;
    void* m_data = nullptr;
    vk::DeviceSize m_size = 0;
};
}
