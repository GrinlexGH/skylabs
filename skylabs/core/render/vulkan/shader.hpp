#pragma once
#include <skylabs/core/render/vulkan/context/context.hpp>

namespace Vulkan {
class CShader
{
public:
    enum class Type : std::uint8_t
    {
        eVertex,
        eFragment,
        eCompute,
        eCount,
    };

    explicit CShader(const CContext& context, Type type, const char* name);
    CShader(const CShader&) = delete;
    CShader(CShader&&) noexcept = default;
    CShader& operator=(const CShader&) = delete;
    CShader& operator=(CShader&&) noexcept = default;
    ~CShader() = default;

    [[nodiscard]] const vk::raii::ShaderModule& operator*() const noexcept { return m_handle; }
    [[nodiscard]] const vk::raii::ShaderModule* operator->() const noexcept { return &m_handle; }

    [[nodiscard]] vk::PipelineShaderStageCreateInfo GetPipelineShaderCreateInfo() const { return m_shaderCreateInfo; }

private:
    vk::raii::ShaderModule m_handle = nullptr;
    vk::PipelineShaderStageCreateInfo m_shaderCreateInfo;
};
}
