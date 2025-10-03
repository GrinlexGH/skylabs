#pragma once
#include <vk_mem_alloc.hpp>

namespace Vulkan {
class CMemoryMapping
{
public:
    CMemoryMapping(
        const vma::UniqueAllocator* allocator,
        const vma::UniqueAllocation* allocation
    );
    CMemoryMapping(const CMemoryMapping&) = delete;
    CMemoryMapping(CMemoryMapping&&) noexcept;
    CMemoryMapping& operator=(const CMemoryMapping&) = delete;
    CMemoryMapping& operator=(CMemoryMapping&&) noexcept;
    ~CMemoryMapping();

    auto operator*() const noexcept -> void* { return m_data; }
    [[nodiscard]] auto GetData() const -> void* { return m_data; }

    // TODO: simple copying function

    auto Clear() -> void;

private:
    const vma::UniqueAllocator* m_allocator = nullptr;
    const vma::UniqueAllocation* m_allocation = nullptr;
    void* m_data = nullptr;
};
}
