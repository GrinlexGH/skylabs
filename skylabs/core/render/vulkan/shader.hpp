#pragma once
#include <skylabs/core/render/vulkan/context/context.hpp>

namespace Vulkan {
enum class ShaderStageBits : std::uint8_t
{
    eVertex = 0,
    eFragment,
    eCompute
};

using ShaderStage = Utils::Flags<ShaderStageBits>;

class CShader
{
public:
    explicit CShader(std::nullptr_t) {}
    explicit CShader(const CContext& context, ShaderStageBits type, const char* name);
    CShader(const CShader&) = delete;
    CShader(CShader&&) noexcept = default;
    CShader& operator=(const CShader&) = delete;
    CShader& operator=(CShader&&) noexcept = default;
    ~CShader() = default;

    [[nodiscard]] const vk::raii::ShaderModule& operator*() const noexcept { return m_handle; }
    [[nodiscard]] const vk::raii::ShaderModule* operator->() const noexcept { return &m_handle; }

    [[nodiscard]] ShaderStage Type() { return m_type; }
    [[nodiscard]] vk::PipelineShaderStageCreateInfo PipelineShaderCreateInfo() const { return m_shaderCreateInfo; }

private:
    ShaderStage m_type;

    vk::raii::ShaderModule m_handle = nullptr;
    vk::PipelineShaderStageCreateInfo m_shaderCreateInfo;
};
}

template <>
struct Utils::FlagTraits<Vulkan::ShaderStageBits>
{
    static constexpr bool isBitmask = true;
    static constexpr Vulkan::ShaderStage allFlags =
        Vulkan::ShaderStageBits::eVertex |
        Vulkan::ShaderStageBits::eFragment |
        Vulkan::ShaderStageBits::eCompute;
};
