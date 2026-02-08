#pragma once
#include <skylabs/core/render/vulkan/render_graph/resource_manager.hpp>

namespace Vulkan {
class IRenderPassCreator
{
public:
    ~IRenderPassCreator() = default;

    virtual void SetupResources(CRGResourceManager& resourceManager) = 0;
};
}
