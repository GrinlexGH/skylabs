#pragma once
#include "skylabs/engine/render/vertex.hpp"
#include "skylabs/engine/render/vulkan/context.hpp"
#include "skylabs/engine/render/vulkan/shader.hpp"

namespace sk::render::vulkan {
struct VertexBufferBinding {
    vk::VertexInputBindingDescription description { };
    std::vector<VertexAttribute> attributes;
};

struct GraphicsPipelineCreateInfo {
    vk::PipelineLayout layout { };
    std::vector<const Shader*> shaders;
    std::vector<VertexBufferBinding> vertexBindings;
    std::vector<vk::DescriptorSetAndBindingMappingEXT> heapMappings;
    vk::PipelineRenderingCreateInfo renderingInfo { };  // TODO: attachment info with blending
    vk::PrimitiveTopology primitiveTopology = vk::PrimitiveTopology::eTriangleList;
    vk::SampleCountFlagBits sampling = vk::SampleCountFlagBits::e1;
};

class GraphicsPipeline {
public:
    explicit GraphicsPipeline(std::nullptr_t) { }
    explicit GraphicsPipeline(const Device& device, const GraphicsPipelineCreateInfo& options = { });

    [[nodiscard]] const vk::raii::Pipeline& operator*() const noexcept { return m_handle; }
    [[nodiscard]] const vk::raii::Pipeline* operator->() const noexcept { return &m_handle; }

    [[nodiscard]] vk::PipelineLayout Layout() const { return m_layout; }

private:
    vk::raii::Pipeline m_handle { nullptr };
    vk::PipelineLayout m_layout { nullptr };
};
}
