#pragma once
#include <skylabs/core/render/vulkan/context/device.hpp>

#include <vk_mem_alloc_raii.hpp>

namespace Vulkan {
class CAllocator
{
public:
    explicit CAllocator(std::nullptr_t) {}
    explicit CAllocator(
        const vk::raii::Instance& instance,
        const vk::raii::PhysicalDevice& physicalDevice,
        const CDevice& device
    );
    CAllocator(const CAllocator&) = delete;
    CAllocator(CAllocator&& other) noexcept = default;
    CAllocator& operator=(const CAllocator&) = delete;
    CAllocator& operator=(CAllocator&& rhs) noexcept = default;
    ~CAllocator() = default;

    [[nodiscard]] auto operator*() const noexcept -> vma::Allocator { return *m_handle; }
    [[nodiscard]] auto operator->() const noexcept -> const vma::Allocator* { return &*m_handle; }

private:
    vma::raii::Allocator m_handle { nullptr };
};
}
