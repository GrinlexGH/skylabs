#pragma once
#include "skylabs/base/filesystem.hpp"
#include "skylabs/core/render/vulkan/command_recording/command_buffer.hpp"
#include "skylabs/core/render/vulkan/in_flight.hpp"
#include "skylabs/core/render/vulkan/pipeline/descriptor_allocator.hpp"
#include "skylabs/core/render/vulkan/pipeline/descriptor_layout_cache.hpp"
#include "skylabs/core/render/vulkan/pipeline/graphics_pipeline.hpp"
#include "skylabs/core/render/vulkan/pipeline/pipeline_layout_cache.hpp"
#include "skylabs/core/render/vulkan/resources/image.hpp"
#include "skylabs/core/render/vulkan/resources/sampler.hpp"

namespace sk::render::vulkan {
class PostProcessPass {
public:
    explicit PostProcessPass(std::nullptr_t) { }
    explicit PostProcessPass(const Device& device, const InFlightContext& inFlightContext,
                             PipelineLayoutCache& pipelineLayoutCache,
                             DescriptorLayoutCache& descriptorLayoutCache,
                             DescriptorAllocator& descriptorAllocator,
                             const filesystem::Filesystem& filesystem,
                             const InFlight<Image>& inAttachment, vk::Format swapchainFormat);
    PostProcessPass(const PostProcessPass&) = delete;
    PostProcessPass(PostProcessPass&&) noexcept = default;
    PostProcessPass& operator=(const PostProcessPass&) = delete;
    PostProcessPass& operator=(PostProcessPass&&) noexcept = default;
    ~PostProcessPass() = default;

    void Draw(const CommandBuffer& cmd, const Image& swapchainImage);
    void Resize(const InFlight<Image>& inAttachment);

private:
    const vk::raii::Device* m_device = nullptr;
    const InFlightContext* m_inFlightContext = nullptr;

    Sampler m_sampler { nullptr };

    InFlight<vk::raii::DescriptorSet> m_swapchainDescriptorSet { nullptr };
    GraphicsPipeline m_pipelineSwapchain { nullptr };
};
}
