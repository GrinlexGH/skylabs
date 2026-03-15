#pragma once
#include <skylabs/core/render/vulkan/context/context.hpp>
#include <skylabs/core/render/vulkan/shader.hpp>
#include <skylabs/core/render/vertex.hpp>

#include <vector>

namespace Vulkan {
struct PipelineInputData
{
    std::vector<vk::DescriptorSetLayout> m_descriptorSets = {};
    std::vector<vk::PushConstantRange> m_pushConstants = {};
};

struct VertexBufferBinding
{
    vk::VertexInputBindingDescription m_description = {};
    std::vector<CVertexAttribute> m_attributes = {};
};

struct GraphicsPipelineCreateInfo
{
    PipelineInputData m_input = {};
    std::vector<const CShader*> m_shaders = {};
    std::vector<VertexBufferBinding> m_vertexBindings = {};
    vk::PipelineRenderingCreateInfo m_renderingInfo = {};
    vk::SampleCountFlagBits m_sampling = vk::SampleCountFlagBits::e1;
};

class CPipeline
{
public:
    explicit CPipeline(std::nullptr_t) {}
    explicit CPipeline(const CContext& context, GraphicsPipelineCreateInfo options = {});
    CPipeline(const CPipeline&) = delete;
    CPipeline(CPipeline&&) noexcept = default;
    CPipeline& operator=(const CPipeline&) = delete;
    CPipeline& operator=(CPipeline&&) noexcept = default;
    ~CPipeline() = default;

    [[nodiscard]] const vk::raii::Pipeline& operator*() const noexcept { return m_handle; }
    [[nodiscard]] const vk::raii::Pipeline* operator->() const noexcept { return &m_handle; }
    [[nodiscard]] const vk::raii::Pipeline& Handle() const noexcept { return m_handle; }

    [[nodiscard]] const vk::raii::PipelineLayout& Layout() { return m_layout; }

private:
    vk::raii::Pipeline m_handle { nullptr };
    vk::raii::PipelineLayout m_layout { nullptr };
};
}
