#pragma once
#include "vulkan.hpp"

namespace Vulkan {
class CAllocator
{
public:
    explicit CAllocator(
        const vk::Instance& instance,
        const vk::PhysicalDevice& physicalDevice,
        const vk::Device& device
    );
    CAllocator(const CAllocator&) = default;
    CAllocator(CAllocator&&) = default;
    CAllocator& operator=(const CAllocator&) = default;
    CAllocator& operator=(CAllocator&&) = default;
    ~CAllocator();

    [[nodiscard]] vma::Allocator GetHandle() const { return m_handle; }

private:
    vma::Allocator m_handle;
};
}
