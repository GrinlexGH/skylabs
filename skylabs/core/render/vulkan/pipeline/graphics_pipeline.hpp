#pragma once
#include "skylabs/core/render/vertex.hpp"
#include "skylabs/core/render/vulkan/pipeline/shader.hpp"

namespace sk::render::vulkan {
struct VertexBufferBinding {
    vk::VertexInputBindingDescription description { };
    std::vector<VertexAttribute> attributes;
};

struct GraphicsPipelineCreateInfo {
    vk::PipelineLayout layout { };
    std::vector<const Shader*> shaders;
    std::vector<VertexBufferBinding> vertexBindings;
    vk::PipelineRenderingCreateInfo renderingInfo { };  // TODO: attachment info with blending
    vk::PrimitiveTopology primitiveTopology = vk::PrimitiveTopology::eTriangleList;
    vk::SampleCountFlagBits sampling = vk::SampleCountFlagBits::e1;
};

class GraphicsPipeline {
public:
    explicit GraphicsPipeline(std::nullptr_t) { }
    explicit GraphicsPipeline(const vk::raii::Device& device,
                              const GraphicsPipelineCreateInfo& options = { });
    GraphicsPipeline(const GraphicsPipeline&) = delete;
    GraphicsPipeline(GraphicsPipeline&&) noexcept = default;
    GraphicsPipeline& operator=(const GraphicsPipeline&) = delete;
    GraphicsPipeline& operator=(GraphicsPipeline&&) noexcept = default;
    ~GraphicsPipeline() = default;

    [[nodiscard]] const vk::raii::Pipeline& operator*() const noexcept { return m_handle; }
    [[nodiscard]] const vk::raii::Pipeline* operator->() const noexcept { return &m_handle; }

    [[nodiscard]] vk::PipelineLayout Layout() const { return m_layout; }

private:
    vk::raii::Pipeline m_handle { nullptr };
    vk::PipelineLayout m_layout { nullptr };
};
}
