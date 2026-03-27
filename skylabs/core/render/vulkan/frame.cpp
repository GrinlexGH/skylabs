#include <skylabs/core/render/vulkan/frame.hpp>

namespace Vulkan {
CFrame::CFrame(const CContext& context) : m_context(&context) {
    m_fence = vk::raii::Fence { *m_context->Device(), { vk::FenceCreateFlagBits::eSignaled } };
    m_imageAvailableSemaphore = vk::raii::Semaphore { *m_context->Device(), vk::SemaphoreCreateInfo {} };
}
}
