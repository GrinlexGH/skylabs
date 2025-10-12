#include "memory_mapping.hpp"

namespace Vulkan {
CMemoryMapping::CMemoryMapping(
    const vma::Allocator& allocator,
    const vma::Allocation& allocation
) : m_allocator(allocator), m_allocation(allocation) {
    if (m_allocation && m_allocator) {
        m_data = m_allocator.mapMemory(m_allocation);
    }
}

CMemoryMapping::CMemoryMapping(CMemoryMapping&& other) noexcept :
    m_allocator(std::exchange(other.m_allocator, nullptr)),
    m_allocation(std::exchange(other.m_allocation, nullptr)),
    m_data(std::exchange(other.m_data, nullptr)) {}

CMemoryMapping& CMemoryMapping::operator=(CMemoryMapping&& rhs) noexcept {
    if (this != &rhs) {
        std::swap(m_allocation, rhs.m_allocation);
        std::swap(m_allocator, rhs.m_allocator);
        std::swap(m_data, rhs.m_data);
    }
    return *this;
}

auto CMemoryMapping::Clear() -> void {
    m_allocator.unmapMemory(m_allocation);
    m_data = nullptr;
    m_allocation = nullptr;
    m_allocator = nullptr;
}

CMemoryMapping::~CMemoryMapping() {
    if (m_data && m_allocator && m_allocation) {
        Clear();
    }
}
}
