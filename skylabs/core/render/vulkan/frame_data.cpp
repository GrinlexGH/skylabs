#include <skylabs/core/render/vulkan/frame_data.hpp>

namespace Vulkan {
CFrameData::CFrameData(const CContext& context) {
    //====================
    m_commandPool = (*context.GetDevice()).createCommandPool(
        {
            vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
            context.GetDevice().GetGraphicsQueue().m_familyIndex
        }
    );

    //====================
    m_commandBuffer = vk::raii::CommandBuffers {
        *context.GetDevice(),
        {m_commandPool, vk::CommandBufferLevel::ePrimary, 1}
    };

    //====================
    m_fence = (*context.GetDevice()).createFence({ vk::FenceCreateFlagBits::eSignaled });

    //====================
    m_imageAvailableSemaphore = (*context.GetDevice()).createSemaphore({});
}
}
