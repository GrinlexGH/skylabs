#include "frame_data.hpp"

namespace Vulkan {
CFrameData::CFrameData(const CRenderContext* context) : m_context(context) {
    const vk::raii::Device& deviceHandle = context->GetDevice()->GetHandle();

    //====================
    m_commandPool = deviceHandle.createCommandPool(
        {
            vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
            m_context->GetDevice()->GetGraphicsQueue().m_familyIndex
        }
    );

    //====================
    m_commandBuffer = vk::raii::CommandBuffers {
        deviceHandle, { m_commandPool, vk::CommandBufferLevel::ePrimary, 1 }
    };

    //====================
    m_fence = deviceHandle.createFence({ vk::FenceCreateFlagBits::eSignaled });

    //====================
    m_imageAvailableSemaphore = deviceHandle.createSemaphore({});
}
}
