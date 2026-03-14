#pragma once
#include <skylabs/core/render/vulkan/context/context.hpp>
#include <skylabs/core/render/vulkan/vertex_format.hpp>
#include <skylabs/core/render/vulkan/shader.hpp>

namespace Vulkan {
class CPipeline
{
public:
    explicit CPipeline(std::nullptr_t) {}
    explicit CPipeline(
        const CContext& context,
        std::span<const CShader*> shaders,
        std::span<const vk::DescriptorSetLayout> descriptorSetLayouts,
        const CVertexFormat& vertexFormat,
        const vk::PipelineRenderingCreateInfo& renderingInfo,
        vk::SampleCountFlagBits sampleCount = vk::SampleCountFlagBits::e1
    );
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
