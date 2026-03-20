#include <skylabs/core/render/vulkan/command_buffer_set.hpp>

namespace Vulkan {
CCommandBufferSet::CCommandBufferSet(
    const CContext& context,
    std::uint32_t familyIndex,
    CommandBuffersCount buffersCount
) : m_context(&context) {
    m_pool = vk::raii::CommandPool { *m_context->Device(), {
        vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        familyIndex
    }};

    Realloc(buffersCount);
}

void CCommandBufferSet::Realloc(CommandBuffersCount newBufferCount) {
    if (newBufferCount.m_primaryCount > 0) {
        m_primaryBuffers = vk::raii::CommandBuffers { *m_context->Device(), {
            m_pool, vk::CommandBufferLevel::ePrimary, newBufferCount.m_primaryCount
        }};
    }

    if (newBufferCount.m_secondaryCount > 0) {
        m_primaryBuffers = vk::raii::CommandBuffers { *m_context->Device(), {
            m_pool, vk::CommandBufferLevel::eSecondary, newBufferCount.m_secondaryCount
        }};
    }
}
}
