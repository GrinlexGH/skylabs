#pragma once
#include "context/context.hpp"

#include <concepts>

namespace Vulkan {
class CBuffer
{
public:
    explicit CBuffer(std::nullptr_t) {}
    CBuffer(
        const CContext* context,
        vk::DeviceSize size,
        const vk::BufferUsageFlags& usage,
        const vk::MemoryPropertyFlags& memoryProperties
    );
    CBuffer(const CBuffer&) = delete;
    CBuffer(CBuffer&&) noexcept = default;
    CBuffer& operator=(const CBuffer&) = delete;
    CBuffer& operator=(CBuffer&&) noexcept = default;
    ~CBuffer();

    auto operator*() const noexcept -> vk::Buffer { return *m_handle; }
    [[nodiscard]] auto GetHandle() const -> vk::Buffer { return *m_handle; }

    auto Clear() -> void;

    auto CopyFromHost(const void* hostData, std::size_t size) -> void;

    template<typename  Func> requires
        std::invocable<Func, void*> &&
        std::same_as<std::invoke_result_t<Func, void*>, void>
    auto WithMappedMemory(Func func) -> void {
        if (!(m_memoryProperties & vk::MemoryPropertyFlagBits::eHostVisible)) {
            throw std::runtime_error("Cannot map memory that is not host visible!"); // TODO: maybe assert?
        }

        const vma::Allocator& allocator = *m_context->GetAllocator();
        void* data = allocator.mapMemory(*m_allocation);
        try {
            func(data);
        } catch (const std::exception&) {
            allocator.unmapMemory(*m_allocation);
            throw;
        }
        allocator.unmapMemory(*m_allocation);
    }

private:
    vma::UniqueBuffer m_handle;
    vma::UniqueAllocation m_allocation;

    vk::MemoryPropertyFlags m_memoryProperties;

    const CContext* m_context = nullptr;
};
}
