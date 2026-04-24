#pragma once
#include <skylabs/core/render/vulkan/context/context.hpp>
#include <skylabs/core/render/vulkan/pipeline/shader.hpp>
#include <skylabs/core/render/vertex.hpp>

#include <vector>

namespace Vulkan {
struct VertexBufferBinding
{
    vk::VertexInputBindingDescription m_description = {};
    std::vector<CVertexAttribute> m_attributes = {};
};

struct GraphicsPipelineCreateInfo
{
    vk::PipelineLayout m_layout = {};
    std::vector<const CShader*> m_shaders = {};
    std::vector<VertexBufferBinding> m_vertexBindings = {};
    vk::PipelineRenderingCreateInfo m_renderingInfo = {}; // TODO: attachment info with blending
    vk::PrimitiveTopology m_primitiveTopology = vk::PrimitiveTopology::eTriangleList;
    vk::SampleCountFlagBits m_sampling = vk::SampleCountFlagBits::e1;
};

class CGraphicsPipeline
{
public:
    explicit CGraphicsPipeline(std::nullptr_t) {}
    explicit CGraphicsPipeline(const CContext& context, GraphicsPipelineCreateInfo options = {});
    CGraphicsPipeline(const CGraphicsPipeline&) = delete;
    CGraphicsPipeline(CGraphicsPipeline&&) noexcept = default;
    CGraphicsPipeline& operator=(const CGraphicsPipeline&) = delete;
    CGraphicsPipeline& operator=(CGraphicsPipeline&&) noexcept = default;
    ~CGraphicsPipeline() = default;

    [[nodiscard]] const vk::raii::Pipeline& operator*() const noexcept { return m_handle; }
    [[nodiscard]] const vk::raii::Pipeline* operator->() const noexcept { return &m_handle; }

    [[nodiscard]] vk::PipelineLayout Layout() { return m_layout; }

private:
    vk::raii::Pipeline m_handle { nullptr };
    vk::PipelineLayout m_layout { nullptr };
};
}
