#pragma once
#include <vector>
#include <skylabs/core/render/render_pass.hpp>

class CRenderGraph
{
public:
    auto AddPass(CRenderPass pass) -> CRenderGraph&;

    [[nodiscard]] auto CreateTexture(CRPTextureDescription description) -> CRPTexture;
    [[nodiscard]] auto CreateBuffer(CRPBufferDescription description) -> CRPBuffer;

private:
    std::vector<CRPTextureDescription> m_textures;
    std::vector<CRPBufferDescription> m_buffers;
};
