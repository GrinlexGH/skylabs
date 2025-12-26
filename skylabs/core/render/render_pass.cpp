#include <skylabs/core/render/render_graph.hpp>

CRenderPass& CRenderPass::AttachTexture(CRPTexture texture, CRPResourceOp op) {

    return *this;
}

CRenderPass& CRenderPass::UseBuffer(CRPBuffer buffer, CRPResourceOp op) {

    return *this;
}

CRenderPass& CRenderPass::SampleTexture(CRPTexture texture) {

    return *this;
}

CRenderPass& CRenderPass::SetExecutionCallback(const std::function<void(CRPContext&)>& callback) {
    m_executionCallback = callback;
    return *this;
}
