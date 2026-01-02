#pragma once
#include <skylabs/core/render/vulkan/context/context.hpp>
#include <skylabs/core/render/vulkan/vertex_format.hpp>

namespace Vulkan {
enum class CBlendPreset
{
    eDefault,
};

class CPipeline
{
public:
    explicit CPipeline(std::nullptr_t) {}
    explicit CPipeline(
        const CContext& context,
        std::span<const vk::PipelineShaderStageCreateInfo> shaderStages,
        std::span<const vk::DescriptorSetLayout> descriptorSetLayouts,
        const CVertexFormat& vertexFormat,
        vk::SampleCountFlagBits sampleCount = vk::SampleCountFlagBits::e1,
        vk::RenderPass renderPass = nullptr
    );
    CPipeline(const CPipeline&) = delete;
    CPipeline(CPipeline&&) noexcept = default;
    CPipeline& operator=(const CPipeline&) = delete;
    CPipeline& operator=(CPipeline&&) noexcept = default;
    ~CPipeline() = default;

    [[nodiscard]] auto operator*() const noexcept -> const vk::raii::Pipeline& { return m_handle; }
    [[nodiscard]] auto GetHandle() const noexcept -> const vk::raii::Pipeline& { return m_handle; }

    [[nodiscard]] auto GetLayout() -> const vk::raii::PipelineLayout& { return m_layout; }

private:
    vk::raii::Pipeline m_handle { nullptr };
    vk::raii::PipelineLayout m_layout { nullptr };
};
}
