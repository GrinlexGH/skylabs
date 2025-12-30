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
        eCount,
    };

    explicit CShader(const CContext& context, Type type, const char* name);
    CShader(const CShader&) = delete;
    CShader(CShader&&) noexcept = default;
    CShader& operator=(const CShader&) = delete;
    CShader& operator=(CShader&&) noexcept = default;
    ~CShader() = default;

    [[nodiscard]] auto operator*() const noexcept -> const vk::raii::ShaderModule& { return m_handle; }
    [[nodiscard]] auto GetHandle() const noexcept -> const vk::raii::ShaderModule& { return m_handle; }

    [[nodiscard]] auto GetPipelineShaderCreateInfo() const -> vk::PipelineShaderStageCreateInfo { return m_shaderCreateInfo; }

private:
    vk::raii::ShaderModule m_handle = nullptr;
    vk::PipelineShaderStageCreateInfo m_shaderCreateInfo;
};
}
