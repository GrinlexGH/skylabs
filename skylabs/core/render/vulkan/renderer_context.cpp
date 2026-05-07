#include <skylabs/core/render/vulkan/renderer_context.hpp>

namespace Vulkan {
CRendererContext::CRendererContext(const IWindow* window) :
    m_context(std::make_unique<CDeviceContext>(window)),
    m_inFlightContext(FRAMES_IN_FLIGHT_COUNT),
    m_swapchain(*m_context, *m_context->Surface(), 2, vk::PresentModeKHR::eMailbox),
    m_pipelineLayoutCache(*m_context),
    m_descriptorLayoutCache(*m_context),
    m_descriptorAllocator(*m_context)
{}
}
