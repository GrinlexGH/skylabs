#pragma once
#include "../context/context.hpp"

namespace Vulkan {
class CRenderPass
{
public:
    explicit CRenderPass(const CContext* context);

private:
    vk::raii::RenderPass m_renderPass = nullptr;

    const CContext* m_context;
};
}
