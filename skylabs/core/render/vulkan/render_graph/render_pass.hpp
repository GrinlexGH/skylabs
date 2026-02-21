#pragma once
#include <skylabs/core/render/vulkan/render_graph/resource_manager.hpp>
#include <skylabs/core/render/vulkan/pipeline.hpp>
#include <skylabs/core/render/vulkan/shader.hpp>

namespace Vulkan::RG {

enum class SampleCount : std::uint8_t
{
    e1 = 0,
    e2,
    e4,
    e8
};

class CRenderPass
{
public:
    explicit CRenderPass(std::nullptr_t) {}
    explicit CRenderPass(CResourceManager& resourceManager, SampleCount sampleCount = SampleCount::e1);
    CRenderPass(const CRenderPass&) = delete;
    CRenderPass(CRenderPass&&) noexcept = default;
    CRenderPass& operator=(const CRenderPass&) = delete;
    CRenderPass& operator=(CRenderPass&&) noexcept = default;

    void AttachVertexShader(CShader vertexShader);
    void AttachFragmentShader(CShader fragmentShader);

    void AttachUniformBuffer(DescriptorHandle handle);

    void BuildPipeline();

private:
    CResourceManager* m_resourceManager = nullptr;

    SampleCount m_sampleCount = SampleCount::e1;
    CShader m_vertexShader { nullptr };
    CShader m_fragmentShader { nullptr };

    CPipeline m_pipeline { nullptr };

    std::vector<vk::DescriptorSetLayoutBinding> m_bindings;
};
}
