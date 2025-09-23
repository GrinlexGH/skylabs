#pragma once
#include "instance.hpp"
#include "device.hpp"

#include <vk_mem_alloc.hpp>

namespace Vulkan {
class CAllocator
{
public:
    explicit CAllocator(std::nullptr_t) {}
    explicit CAllocator(
        const CInstance& instance,
        const vk::PhysicalDevice& physicalDevice,
        const CDevice& device
    );
    CAllocator(const CAllocator&) = delete;
    CAllocator(CAllocator&& other) noexcept;
    CAllocator& operator=(const CAllocator&) = delete;
    CAllocator& operator=(CAllocator&& rhs) noexcept;
    ~CAllocator();

    auto operator*() const noexcept -> const vma::Allocator& { return m_handle; }
    [[nodiscard]] auto GetHandle() const -> vma::Allocator { return m_handle; }

private:
    vma::Allocator m_handle = nullptr;
};
}
