#pragma once
#include "vulkan.hpp"

namespace Vulkan {
class CAllocator
{
public:
    CAllocator() = default;
    CAllocator(const CAllocator&) = default;
    CAllocator(CAllocator&&) = default;
    CAllocator& operator=(const CAllocator&) = default;
    CAllocator& operator=(CAllocator&&) = default;
    ~CAllocator();

    void Create(const vk::Instance& instance, const vk::PhysicalDevice& physicalDevice, const vk::Device& device);
    [[nodiscard]] vma::Allocator GetHandle() const { return m_handle; }

private:
    vma::Allocator m_handle;
};
}
