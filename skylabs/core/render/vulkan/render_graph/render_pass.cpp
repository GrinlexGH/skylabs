#include <skylabs/core/render/vulkan/render_graph/render_pass.hpp>

namespace Vulkan::RG {
CRenderPass::CRenderPass(
    CResourceManager& resourceManager,
    SampleCount sampleCount
) : m_resourceManager(&resourceManager), m_sampleCount(sampleCount)
{ }

void CRenderPass::AttachVertexShader(CShader vertexShader) {
    assert(vertexShader.Type() == CShader::Stage::eVertex);
    m_vertexShader = std::move(vertexShader);
}

void CRenderPass::AttachFragmentShader(CShader fragmentShader) {
    assert(fragmentShader.Type() == CShader::Stage::eFragment);
    m_fragmentShader = std::move(fragmentShader);
}

void CRenderPass::AttachUniformBuffer(DescriptorHandle handle) {
    
}

void CRenderPass::BuildPipeline() {
    m_pipeline = CPipeline {
        // m_resourceManager->m_context,
        // { m_vertexShader.PipelineShaderCreateInfo(), m_fragmentShader.PipelineShaderCreateInfo() },
        nullptr
    };
}
}
