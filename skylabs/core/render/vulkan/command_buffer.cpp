#include <skylabs/core/render/vulkan/command_buffer.hpp>

namespace Vulkan {
CCommandBuffer::CCommandBuffer(const vk::raii::CommandBuffer& commandBuffer) :
    m_handle(&commandBuffer)
{}
}
