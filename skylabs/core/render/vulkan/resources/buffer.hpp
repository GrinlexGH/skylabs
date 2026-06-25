#pragma once
import skylabs.vulkan.context;

namespace Vulkan {
enum class MemoryLocation : std::uint8_t {
    eDeviceOnly = 0,
    eHostVisible,
};

class CBuffer
{
public:
    explicit CBuffer(std::nullptr_t) {}
    CBuffer(
        const CContext& context,
        vk::DeviceSize size,
        vk::BufferUsageFlags usage,
        MemoryLocation location
    );
    CBuffer(const CBuffer&) = delete;
    CBuffer(CBuffer&&) noexcept = default;
    CBuffer& operator=(const CBuffer&) = delete;
    CBuffer& operator=(CBuffer&&) noexcept = default;
    ~CBuffer() = default;

    [[nodiscard]] const vma::raii::Buffer& operator*() const noexcept { return m_handle; }
    [[nodiscard]] const vma::raii::Buffer* operator->() const noexcept { return &m_handle; }

    [[nodiscard]] void* Data() const noexcept { return m_data; }
    [[nodiscard]] std::size_t Size() const noexcept { return static_cast<std::size_t>(m_size); }
    [[nodiscard]] std::span<std::byte> Span() const {
        return m_data
            ? std::span { static_cast<std::byte*>(m_data), static_cast<std::size_t>(m_size) }
            : std::span<std::byte> {};
    }

    [[nodiscard]] const vma::raii::VirtualBlock& VirtualBlock() const noexcept { return m_memoryBlock; }

    [[nodiscard]] vk::BufferUsageFlags Usage() const noexcept { return m_usage; }

private:
    vma::raii::Buffer m_handle { nullptr };
    vma::raii::VirtualBlock m_memoryBlock { nullptr };

    void* m_data = nullptr;
    vk::DeviceSize m_size = 0;
    vk::BufferUsageFlags m_usage;
};
}
