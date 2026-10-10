#pragma once
#include <vk_mem_alloc_raii.hpp>

#include "skylabs/engine/render/vulkan/context/device.hpp"

namespace sk::render::vulkan {
enum class MemoryLocation : std::uint8_t {
    eDeviceOnly = 0,
    eHostVisible,
};

class Buffer {
public:
    explicit Buffer(std::nullptr_t) { }
    Buffer(const vma::raii::Allocator& allocator, const Device& device, vk::DeviceSize size,
           const vk::BufferUsageFlags2& usage, MemoryLocation location);
    Buffer(const Buffer&) = delete;
    Buffer(Buffer&&) noexcept = default;
    Buffer& operator=(const Buffer&) = delete;
    Buffer& operator=(Buffer&&) noexcept = default;
    ~Buffer() = default;

    [[nodiscard]] const vma::raii::Buffer& operator*() const noexcept { return m_handle; }
    [[nodiscard]] const vma::raii::Buffer* operator->() const noexcept { return &m_handle; }

    [[nodiscard]] void* Data() const noexcept { return m_data; }
    [[nodiscard]] std::size_t Size() const noexcept { return static_cast<std::size_t>(m_size); }
    [[nodiscard]] std::span<std::byte> Span() const {
        return m_data ? std::span { static_cast<std::byte*>(m_data), static_cast<std::size_t>(m_size) }
                      : std::span<std::byte> { };
    }

    [[nodiscard]] const vma::raii::VirtualBlock& VirtualBlock() const noexcept { return m_memoryBlock; }

    [[nodiscard]] vk::BufferUsageFlags2 Usage() const noexcept { return m_usage; }
    [[nodiscard]] vk::DeviceAddress Address() const noexcept { return m_address; }

private:
    vma::raii::Buffer m_handle { nullptr };
    vma::raii::VirtualBlock m_memoryBlock { nullptr };

    void* m_data = nullptr;
    vk::DeviceSize m_size = 0;
    vk::BufferUsageFlags2 m_usage;
    vk::DeviceAddress m_address = 0;
};
}
