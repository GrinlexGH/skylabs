#pragma once
#include <skylabs/core/render/vulkan/context/context.hpp>

namespace Vulkan {
class CShader
{
public:
    enum class Stage : std::uint8_t
    {
        eVertex,
        eFragment,
        eCompute,
        eCount,
    };

    explicit CShader(std::nullptr_t) {}
    explicit CShader(const CContext& context, Stage type, const char* name);
    CShader(const CShader&) = delete;
    CShader(CShader&&) noexcept = default;
    CShader& operator=(const CShader&) = delete;
    CShader& operator=(CShader&&) noexcept = default;
    ~CShader() = default;

    [[nodiscard]] const vk::raii::ShaderModule& operator*() const noexcept { return m_handle; }
    [[nodiscard]] const vk::raii::ShaderModule* operator->() const noexcept { return &m_handle; }

    [[nodiscard]] Stage Type() { return m_type; }
    [[nodiscard]] vk::PipelineShaderStageCreateInfo PipelineShaderCreateInfo() const { return m_shaderCreateInfo; }

private:
    enum Stage m_type = Stage::eVertex;

    vk::raii::ShaderModule m_handle = nullptr;
    vk::PipelineShaderStageCreateInfo m_shaderCreateInfo;
};
}
