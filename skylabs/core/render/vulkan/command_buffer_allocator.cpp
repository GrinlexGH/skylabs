#include <skylabs/core/render/vulkan/command_buffer_allocator.hpp>

namespace Vulkan {
CCommandBufferAllocator::CCommandBufferAllocator(const CDeviceContext& context, std::uint32_t familyIndex) : m_context(&context) {
    m_pool = vk::raii::CommandPool { *m_context->Device(), {
        vk::CommandPoolCreateFlagBits::eResetCommandBuffer, familyIndex
    }};
}

std::vector<CCommandBuffer> CCommandBufferAllocator::Allocate(vk::CommandBufferLevel level, std::uint32_t count) const {
    auto cmds = vk::raii::CommandBuffers { *m_context->Device(), {
        m_pool, level, count
    }};

    std::vector<CCommandBuffer> outCmds;
    outCmds.reserve(cmds.size());
    for (auto& cmd : cmds) {
        outCmds.emplace_back(*m_context, std::move(cmd));
    }

    return outCmds;
}
}
