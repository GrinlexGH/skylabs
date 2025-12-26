#pragma once
#include <skylabs/core/render/vulkan/memory/memory_mapping.hpp>
#include <skylabs/core/render/vulkan/context/context.hpp>

namespace Vulkan {
class CHostBuffer
{
public:
    explicit CHostBuffer(std::nullptr_t) {}
    CHostBuffer(
        const CContext& context,
        vk::DeviceSize size,
        const vk::BufferUsageFlags& usage
    );
    CHostBuffer(const CHostBuffer&) = delete;
    CHostBuffer(CHostBuffer&&) noexcept = default;
    CHostBuffer& operator=(const CHostBuffer&) = delete;
    CHostBuffer& operator=(CHostBuffer&&) noexcept = default;
    ~CHostBuffer() = default;

    [[nodiscard]] auto operator*() const noexcept -> vk::Buffer { return *m_handle; }
    [[nodiscard]] auto GetHandle() const noexcept -> vk::Buffer { return *m_handle; }

    auto Clear() -> void;

    auto Map() -> CMemoryMapping { return { m_context->GetAllocator(), *m_allocation }; }

private:
    vma::UniqueBuffer m_handle;
    vma::UniqueAllocation m_allocation;

    const CContext* m_context = nullptr;
};
}
