#include <skylabs/core/render/vulkan/frame_data.hpp>

namespace Vulkan {
CFrameData::CFrameData(const CContext& context) : m_context(&context) {
    //====================
    m_commandPool = vk::raii::CommandPool {
        *m_context->GetDevice(),
        {
            vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
            context.GetDevice().GetGraphicsQueue().m_familyIndex
        }
    };

    //====================
    m_commandBuffer = vk::raii::CommandBuffers {
        *m_context->GetDevice(),
        {m_commandPool, vk::CommandBufferLevel::ePrimary, 1}
    };

    //====================
    m_fence = vk::raii::Fence { *m_context->GetDevice(), { vk::FenceCreateFlagBits::eSignaled } };

    //====================
    m_imageAvailableSemaphore = vk::raii::Semaphore { *m_context->GetDevice(), vk::SemaphoreCreateInfo {} };
}
}
