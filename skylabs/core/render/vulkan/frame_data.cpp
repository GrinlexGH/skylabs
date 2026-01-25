#include <skylabs/core/render/vulkan/frame_data.hpp>

namespace Vulkan {
CFrameData::CFrameData(const CContext& context) : m_context(&context) {
    m_computeCommandPool = vk::raii::CommandPool {
        *m_context->Device(),
        {
            vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
            context.Device().ComputeQueue().FamilyIndex()
        }
    };

    m_computeCommandBuffers = vk::raii::CommandBuffers {
        *m_context->Device(),
        { m_computeCommandPool, vk::CommandBufferLevel::ePrimary, 1 }
    };

    m_graphicsCommandPool = vk::raii::CommandPool {
        *m_context->Device(),
        {
            vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
            context.Device().GraphicsQueue().FamilyIndex()
        }
    };

    m_graphicsCommandBuffers = vk::raii::CommandBuffers {
        *m_context->Device(),
        { m_graphicsCommandPool, vk::CommandBufferLevel::ePrimary, 2 }
    };

    m_fence = vk::raii::Fence { *m_context->Device(), { vk::FenceCreateFlagBits::eSignaled } };
    m_imageAvailableSemaphore = vk::raii::Semaphore { *m_context->Device(), vk::SemaphoreCreateInfo {} };

    for (std::size_t i = 0; i < 2; ++i) {
        m_semaphores.emplace_back(*m_context->Device(), vk::SemaphoreCreateInfo {});
    }
}
}
