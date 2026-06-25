#pragma once
#include <skylabs/core/render/vulkan/command_recording/command_buffer.hpp>

namespace Vulkan {
class CCommandBufferAllocator
{
public:
    explicit CCommandBufferAllocator(std::nullptr_t) {}
    explicit CCommandBufferAllocator(const CContext& context, std::uint32_t familyIndex);
    CCommandBufferAllocator(const CCommandBufferAllocator&) = delete;
    CCommandBufferAllocator(CCommandBufferAllocator&&) noexcept = default;
    CCommandBufferAllocator& operator=(const CCommandBufferAllocator&) = delete;
    CCommandBufferAllocator& operator=(CCommandBufferAllocator&&) noexcept = default;
    ~CCommandBufferAllocator() = default;

    std::vector<CCommandBuffer> Allocate(vk::CommandBufferLevel level, std::uint32_t count) const;

private:
    const CContext* m_context = nullptr;
    vk::raii::CommandPool m_pool { nullptr };
};
}
