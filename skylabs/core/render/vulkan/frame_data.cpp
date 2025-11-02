#include <skylabs/core/render/vulkan/frame_data.hpp>

namespace Vulkan {
CFrameData::CFrameData(const CContext& context) {
    //====================
    m_commandPool = vk::raii::CommandPool {
        *context.GetDevice(),
        {
            vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
            context.GetDevice().GetGraphicsQueue().m_familyIndex
        }
    };

    //====================
    m_commandBuffer = vk::raii::CommandBuffers {
        *context.GetDevice(),
        {m_commandPool, vk::CommandBufferLevel::ePrimary, 1}
    };

    //====================
    m_fence = vk::raii::Fence { *context.GetDevice(), { vk::FenceCreateFlagBits::eSignaled } };

    //====================
    m_imageAvailableSemaphore = vk::raii::Semaphore { *context.GetDevice(), vk::SemaphoreCreateInfo {} };
}
}
