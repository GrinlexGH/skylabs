#pragma once
#include <skylabs/core/render/render_pass_pipeline.hpp>
#include <skylabs/core/render/render_pass_buffer.hpp>
#include <skylabs/core/render/render_pass_texture.hpp>


class CRPContext
{
public:
    auto BindPipeline(CRPPipeline pipeline) -> void;
    auto BindVertexBuffer(CRPBuffer pipeline) -> void;
    auto BindIndexBuffer(CRPBuffer pipeline) -> void;

    auto Draw(std::size_t vertexCount) -> void;
    auto DrawIndexed(std::size_t indexCount) -> void;

    auto Present(CRPTexture texture) -> void;
};
