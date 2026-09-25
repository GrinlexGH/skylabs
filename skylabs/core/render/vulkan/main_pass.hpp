#pragma once
#include "../../../engine/filesystem.hpp"
#include "../../../engine/render/vulkan/image.hpp"
#include "skylabs/base/utils.hpp"
#include "skylabs/core/render/render_object.hpp"
#include "skylabs/core/render/vulkan/command_recording/command_buffer.hpp"
#include "skylabs/core/render/vulkan/context/allocator.hpp"
#include "skylabs/core/render/vulkan/in_flight.hpp"
#include "skylabs/core/render/vulkan/pipeline/descriptor_allocator.hpp"
#include "skylabs/core/render/vulkan/pipeline/descriptor_layout_cache.hpp"
#include "skylabs/core/render/vulkan/pipeline/graphics_pipeline.hpp"
#include "skylabs/core/render/vulkan/pipeline/pipeline_layout_cache.hpp"
#include "skylabs/core/render/vulkan/resources/sampler.hpp"
#include "skylabs/core/render/vulkan/submesh.hpp"

namespace sk::render::vulkan {
struct MVP {
    glm::mat4 view;
    glm::mat4 proj;
};

class MainPass {
public:
    explicit MainPass(std::nullptr_t) { }
    explicit MainPass(const Device& device, const InFlightContext& inFlightContext,
                      const Allocator& allocator, PipelineLayoutCache& pipelineLayoutCache,
                      DescriptorLayoutCache& descriptorLayoutCache,
                      DescriptorAllocator& descriptorAllocator,
                      const filesystem::Filesystem& filesystem, utils::Extent2D renderExtent);
    MainPass(const MainPass&) = delete;
    MainPass(MainPass&&) noexcept = default;
    MainPass& operator=(const MainPass&) = delete;
    MainPass& operator=(MainPass&&) noexcept = default;
    ~MainPass() = default;

    void WriteDescriptors(const std::vector<Image>& textures);
    void Draw(const CommandBuffer& cmd, const Buffer& vertexBuffer, const Buffer& indexBuffer,
              std::span<const SubMesh> meshes, const std::vector<sk::RenderObject>& objects);
    void Resize(utils::Extent2D newExtent);

    InFlight<Image>& MainAttachment() { return m_mainColor; }
    InFlight<Image>& MainMSAAAttachment() { return m_mainColorMSAA; }
    InFlight<Image>& DepthMSAAAttachment() { return m_mainDepthMSAA; }
    InFlight<Buffer>& GetMVP() { return m_mvp; }

private:
    const Device* m_device = nullptr;
    const Allocator* m_allocator = nullptr;
    const InFlightContext* m_inFlightContext = nullptr;

    Sampler m_nearestSampler { nullptr };

    InFlight<Image> m_mainColor { nullptr };
    InFlight<Image> m_mainColorMSAA { nullptr };
    InFlight<Image> m_mainDepthMSAA { nullptr };

    InFlight<Buffer> m_mvp { nullptr };

    InFlight<vk::raii::DescriptorSet> m_mainDescriptorSet { nullptr };

    GraphicsPipeline m_pipeline { nullptr };
};
}
