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
    Buffer(const Device& device, const vma::raii::Allocator& allocator, vk::DeviceSize size,
           MemoryLocation location, vk::BufferUsageFlags2 usage = { },
           const std::optional<vk::DeviceSize>& virtualAllocationSize = std::nullopt);
    Buffer(const Buffer&) = delete;
    Buffer(Buffer&&) noexcept = default;
    Buffer& operator=(const Buffer&) = delete;
    Buffer& operator=(Buffer&&) noexcept = default;
    ~Buffer() = default;

    [[nodiscard]] const vma::raii::Buffer& operator*() const noexcept { return m_handle; }
    [[nodiscard]] const vma::raii::Buffer* operator->() const noexcept { return &m_handle; }

    [[nodiscard]] vk::DeviceAddress DeviceAddress() const noexcept { return m_address; }
    [[nodiscard]] void* HostAddress() const noexcept { return m_data; }
    [[nodiscard]] std::size_t Size() const noexcept { return m_size; }

    [[nodiscard]] const vma::raii::VirtualBlock& VirtualBlock() const noexcept { return m_memoryBlock; }

private:
    vma::raii::Buffer m_handle { nullptr };
    vma::raii::VirtualBlock m_memoryBlock { nullptr };

    void* m_data = nullptr;
    vk::DeviceSize m_size = 0;
    vk::DeviceAddress m_address = 0;
};
}
