#pragma once
#include "../render_context.hpp"

#include <vulkan/vulkan_raii.hpp>

namespace Vulkan {
class CRenderPass
{
public:
    explicit CRenderPass(const CRenderContext* context);

private:
    vk::raii::RenderPass m_renderPass = nullptr;

    const CRenderContext* m_context;
};
}
