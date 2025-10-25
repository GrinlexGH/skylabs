#pragma once
#include <vk_mem_alloc.hpp>

namespace Vulkan {
class CMemoryMapping
{
public:
    CMemoryMapping(
        const vma::Allocator& allocator,
        const vma::Allocation& allocation
    );
    CMemoryMapping(const CMemoryMapping&) = delete;
    CMemoryMapping(CMemoryMapping&&) noexcept;
    CMemoryMapping& operator=(const CMemoryMapping&) = delete;
    CMemoryMapping& operator=(CMemoryMapping&&) noexcept;
    ~CMemoryMapping();

    auto operator*() const noexcept -> void* { return m_data; }
    [[nodiscard]] auto GetData() const -> void* { return m_data; }
    [[nodiscard]] auto GetSize() const -> size_t { return m_size; }

    auto Clear() -> void;

    // TODO: simple copying function?

private:
    vma::Allocator m_allocator = nullptr;
    vma::Allocation m_allocation = nullptr;
    void* m_data = nullptr;
    vk::DeviceSize m_size = 0;
};
}
