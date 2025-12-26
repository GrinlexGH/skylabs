#pragma once
#include <skylabs/core/render/render_pass_texture.hpp>
#include <skylabs/core/render/render_pass_buffer.hpp>
#include <skylabs/core/render/render_pass_context.hpp>

#include <functional>

enum class CRPResourceOp : std::uint8_t
{
    eRead,
    eWrite,
    eReadWrite,
};

class CRenderPass
{
public:
    auto AttachTexture(CRPTexture texture, CRPResourceOp op) -> CRenderPass&;
    auto UseBuffer(CRPBuffer buffer, CRPResourceOp) -> CRenderPass&;
    auto SampleTexture(CRPTexture texture) -> CRenderPass&;
    auto SetExecutionCallback(const std::function<void(CRPContext&)>& callback) -> CRenderPass&;

private:
    std::function<void(CRPContext&)> m_executionCallback;
};
