#pragma once
#include <skylabs/core/render/vulkan/command_buffer_set.hpp>

namespace Vulkan::RG {
struct PassRequirements
{
    CImage& m_image;
    Usage m_usage;
    Usage m_usageAfter = Usage::eNone;

    PassRequirements(CImage& img, Usage usage, Usage usageAfter = Usage::eNone) : m_image(img), m_usage(usage), m_usageAfter(usageAfter) {}
};

struct WaitSemaphore
{
    vk::Semaphore m_semaphore;
    vk::PipelineStageFlags m_stage;
};

struct PassSync
{
    std::vector<vk::Semaphore> m_signalSemaphores;
    std::vector<WaitSemaphore> m_waitSemaphores;
    vk::Fence m_fence = nullptr;
};

enum class PassType : std::uint8_t
{
    eGraphics,
    eCompute
};

struct Pass
{
    PassSync m_sync {};
    PassType m_type = PassType::eGraphics;
    std::vector<PassRequirements> m_requirements;
    std::function<void(const CCommandBuffer&)> m_execute;
};

class CRenderGraph
{
public:
    explicit CRenderGraph(std::nullptr_t) {}
    explicit CRenderGraph(const CContext& context, unsigned int inFlightCount) :
        m_context(&context),
        m_inFlightCount(inFlightCount),
        m_graphicsQueues(context, context.Device().GraphicsQueue().FamilyIndex(), { 3 * inFlightCount }),
        m_computeQueues(context, context.Device().ComputeQueue().FamilyIndex(), { 1 * inFlightCount })
    {}
    CRenderGraph(const CRenderGraph&) = delete;
    CRenderGraph(CRenderGraph&&) noexcept = default;
    CRenderGraph& operator=(const CRenderGraph&) = delete;
    CRenderGraph& operator=(CRenderGraph&&) noexcept = default;
    ~CRenderGraph() = default;

    void Clear() { m_passes.clear(); }
    void AddPass(Pass pass);
    void Execute();

private:
    const CContext* m_context = nullptr;

    unsigned int m_inFlightCount = 1;
    unsigned int m_inFlightIndex = 0;

    CCommandBufferSet m_graphicsQueues { nullptr };
    CCommandBufferSet m_computeQueues { nullptr };
    std::vector<Pass> m_passes;
};
}
