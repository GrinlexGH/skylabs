#include <skylabs/core/render/vulkan/frame_data.hpp>

namespace Vulkan {
CFrameData::CFrameData(const CContext& context) : m_context(&context) {
    m_fence = vk::raii::Fence { *m_context->Device(), { vk::FenceCreateFlagBits::eSignaled } };
    m_imageAvailableSemaphore = vk::raii::Semaphore { *m_context->Device(), vk::SemaphoreCreateInfo {} };

    for (std::size_t i = 0; i < 3; ++i) {
        m_semaphores.emplace_back(*m_context->Device(), vk::SemaphoreCreateInfo {});
    }
}
}
