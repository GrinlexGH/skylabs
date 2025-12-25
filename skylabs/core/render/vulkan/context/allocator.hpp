#pragma once
#include <skylabs/core/render/vulkan/context/instance.hpp>
#include <skylabs/core/render/vulkan/context/device.hpp>

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
    CAllocator(CAllocator&& other) noexcept = default;
    CAllocator& operator=(const CAllocator&) = delete;
    CAllocator& operator=(CAllocator&& rhs) noexcept = default;
    ~CAllocator() = default;

    auto operator*() const noexcept -> vma::Allocator { return *m_handle; }
    [[nodiscard]] auto GetHandle() const -> vma::Allocator { return *m_handle; }

private:
    vma::UniqueAllocator m_handle;
};
}
