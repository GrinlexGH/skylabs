#pragma once
#include "skylabs/engine/render/vulkan/command_buffer.hpp"

namespace sk::render::vulkan {
class CommandBufferAllocator {
public:
    explicit CommandBufferAllocator(std::nullptr_t) { }
    explicit CommandBufferAllocator(const vk::raii::Device& device, std::uint32_t familyIndex);
    CommandBufferAllocator(const CommandBufferAllocator&) = delete;
    CommandBufferAllocator(CommandBufferAllocator&&) noexcept = default;
    CommandBufferAllocator& operator=(const CommandBufferAllocator&) = delete;
    CommandBufferAllocator& operator=(CommandBufferAllocator&&) noexcept = default;
    ~CommandBufferAllocator() = default;

    std::vector<CommandBuffer> Allocate(vk::CommandBufferLevel level, std::uint32_t count) const;

private:
    const vk::raii::Device* m_device = nullptr;
    vk::raii::CommandPool m_pool { nullptr };
};
}
