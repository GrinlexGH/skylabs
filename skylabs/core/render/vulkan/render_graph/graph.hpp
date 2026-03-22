#pragma once
#include <skylabs/core/render/vulkan/render_graph/resource_manager.hpp>
#include <skylabs/core/render/vulkan/command_buffer.hpp>

namespace Vulkan::RG {
struct PassRequirements {
    Vulkan::CImage* m_image;
    Vulkan::Usage m_usage;
};

struct Pass
{
    const CCommandBuffer& m_cmd;
    std::vector<PassRequirements> m_requirements;
    std::function<void()> m_execute;
};

class CRenderGraph
{
public:
    explicit CRenderGraph(std::nullptr_t) {}
    CRenderGraph(const CRenderGraph&) = delete;
    CRenderGraph(CRenderGraph&&) noexcept = default;
    CRenderGraph& operator=(const CRenderGraph&) = delete;
    CRenderGraph& operator=(CRenderGraph&&) noexcept = default;
    ~CRenderGraph() = default;

    void AddPass(Pass pass);
    void Execute();

private:
    std::vector<Pass> m_passes;
};
}