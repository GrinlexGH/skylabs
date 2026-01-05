#include <skylabs/core/render/vulkan/frame_data.hpp>

namespace Vulkan {
CFrameData::CFrameData(const CContext& context) : m_context(&context) {
    //====================
    m_commandPool = vk::raii::CommandPool {
        *m_context->Device(),
        {
            vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
            context.Device().GraphicsQueue().m_familyIndex
        }
    };

    //====================
    m_commandBuffer = vk::raii::CommandBuffers {
        *m_context->Device(),
        {m_commandPool, vk::CommandBufferLevel::ePrimary, 1}
    };

    //====================
    m_fence = vk::raii::Fence { *m_context->Device(), { vk::FenceCreateFlagBits::eSignaled } };

    //====================
    m_imageAvailableSemaphore = vk::raii::Semaphore { *m_context->Device(), vk::SemaphoreCreateInfo {} };
}
}
