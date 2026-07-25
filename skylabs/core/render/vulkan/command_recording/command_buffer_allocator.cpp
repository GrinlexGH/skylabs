#include <skylabs/core/render/vulkan/command_recording/command_buffer_allocator.hpp>

namespace Vulkan {
CCommandBufferAllocator::CCommandBufferAllocator(const vk::raii::Device& device, std::uint32_t familyIndex) : m_device(&device) {
    m_pool = vk::raii::CommandPool { *m_device, {
        vk::CommandPoolCreateFlagBits::eResetCommandBuffer, familyIndex
    }};
}

std::vector<CCommandBuffer> CCommandBufferAllocator::Allocate(vk::CommandBufferLevel level, std::uint32_t count) const {
    auto cmds = vk::raii::CommandBuffers { *m_device, {
        m_pool, level, count
    }};

    std::vector<CCommandBuffer> outCmds;
    outCmds.reserve(cmds.size());
    for (auto& cmd : cmds) {
        outCmds.emplace_back(*m_device, std::move(cmd));
    }

    return outCmds;
}
}
