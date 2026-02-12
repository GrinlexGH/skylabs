#pragma once
#include <skylabs/core/render/vulkan/render_graph/resource_manager.hpp>

namespace Vulkan::RG {
class IRenderPassCreator
{
public:
    ~IRenderPassCreator() = default;

    void SetupResources(CResourceManager& resourceManager);

private:
    
};
}
