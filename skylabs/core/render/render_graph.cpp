#include <skylabs/core/render/render_graph.hpp>

CRPTexture CRenderGraph::CreateTexture(const CRPTextureDescription description) {
    const std::uint32_t handle = m_textures.size();
    m_textures.push_back(description);
    return { handle };
}

CRPBuffer CRenderGraph::CreateBuffer(const CRPBufferDescription description) {
    const std::uint32_t handle = m_buffers.size();
    m_buffers.push_back(description);
    return { handle };
}

CRenderGraph& CRenderGraph::AddPass(CRenderPass pass) {
    return *this;
}
