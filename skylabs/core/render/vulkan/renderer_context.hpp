#pragma once
#include <skylabs/core/render/vulkan/in_flight.hpp>
#include <skylabs/core/render/vulkan/platform/swapchain.hpp>
#include <skylabs/core/render/vulkan/pipeline/pipeline_layout_cache.hpp>
#include <skylabs/core/render/vulkan/pipeline/descriptor_layout_cache.hpp>
#include <skylabs/core/render/vulkan/pipeline/descriptor_allocator.hpp>

namespace Vulkan {
class CRendererContext
{
public:
    explicit CRendererContext(std::nullptr_t) {}
    explicit CRendererContext(const IWindow* window);
    CRendererContext(const CRendererContext&) = delete;
    CRendererContext(CRendererContext&&) noexcept = default;
    CRendererContext& operator=(const CRendererContext&) = delete;
    CRendererContext& operator=(CRendererContext&&) noexcept = default;
    ~CRendererContext() = default;

    CDeviceContext& DeviceContext() const { return *m_context; }
    CInFlightContext& InFlightContext() { return m_inFlightContext; }
    CSwapchain& Swapchain() { return m_swapchain; }
    CPipelineLayoutCache& PipelineLayoutCache() { return m_pipelineLayoutCache; }
    CDescriptorLayoutCache& DescriptorLayoutCache() { return m_descriptorLayoutCache; }
    CDescriptorAllocator& DescriptorAllocator() { return m_descriptorAllocator; }

private:
    static constexpr auto FRAMES_IN_FLIGHT_COUNT = 1;

    std::unique_ptr<CDeviceContext> m_context { nullptr }; // Do not lose the pointer on move
    CInFlightContext m_inFlightContext { nullptr };

    CSwapchain m_swapchain { nullptr };

    CPipelineLayoutCache m_pipelineLayoutCache { nullptr };
    CDescriptorLayoutCache m_descriptorLayoutCache { nullptr };
    CDescriptorAllocator m_descriptorAllocator { nullptr };
};
}
