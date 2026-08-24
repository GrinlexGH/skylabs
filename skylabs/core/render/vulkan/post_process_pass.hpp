#pragma once
#include <skylabs/public/filesystem.hpp>
#include <skylabs/core/render/vulkan/command_recording/command_buffer.hpp>
#include <skylabs/core/render/vulkan/pipeline/graphics_pipeline.hpp>
#include <skylabs/core/render/vulkan/in_flight.hpp>
#include <skylabs/core/render/vulkan/pipeline/pipeline_layout_cache.hpp>
#include <skylabs/core/render/vulkan/pipeline/descriptor_layout_cache.hpp>
#include <skylabs/core/render/vulkan/pipeline/descriptor_allocator.hpp>
#include <skylabs/core/render/vulkan/resources/sampler.hpp>
#include <skylabs/core/render/vulkan/resources/image.hpp>

namespace Vulkan {
class CPostProcessPass
{
public:
    explicit CPostProcessPass(std::nullptr_t) {}
    explicit CPostProcessPass(
        const CDevice& device,
        const CInFlightContext& inFlightContext,
        CPipelineLayoutCache& pipelineLayoutCache,
        CDescriptorLayoutCache& descriptorLayoutCache,
        CDescriptorAllocator& descriptorAllocator,
        const CFilesystem& filesystem,
        const InFlight<CImage>& inAttachment,
        vk::Format swapchainFormat
    );
    CPostProcessPass(const CPostProcessPass&) = delete;
    CPostProcessPass(CPostProcessPass&&) noexcept = default;
    CPostProcessPass& operator=(const CPostProcessPass&) = delete;
    CPostProcessPass& operator=(CPostProcessPass&&) noexcept = default;
    ~CPostProcessPass() = default;

    void Draw(const CCommandBuffer& cmd, const CImage& swapchainImage);
    void Resize(const InFlight<CImage>& inAttachment);

private:
    const vk::raii::Device* m_device = nullptr;
    const CInFlightContext* m_inFlightContext = nullptr;

    CSampler m_sampler { nullptr };

    InFlight<vk::raii::DescriptorSet> m_swapchainDescriptorSet { nullptr };
    CGraphicsPipeline m_pipelineSwapchain { nullptr };
};
}
