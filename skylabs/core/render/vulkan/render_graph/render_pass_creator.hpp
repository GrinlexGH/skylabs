#pragma once
#include <skylabs/core/render/vulkan/render_graph/resource_manager.hpp>

namespace Vulkan::RG {
class IRenderPassCreator
{
public:
    ~IRenderPassCreator() = default;

    virtual void SetupResources(CResourceManager& resourceManager) = 0;
};
}
