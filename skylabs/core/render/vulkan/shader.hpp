#pragma once
#include <skylabs/core/render/vulkan/context/context.hpp>

#include <frozen/map.h>

namespace Vulkan {
enum class ShaderStageBits : std::uint8_t
{
    eVertex = 1 << 0,
    eFragment = 1 << 1,
    eCompute = 1 << 2,
};

using ShaderStage = Utils::Flags<ShaderStageBits>;

constexpr inline frozen::map<ShaderStageBits, vk::ShaderStageFlagBits, 3> g_shaderStageMap
{
    { Vulkan::ShaderStageBits::eVertex, vk::ShaderStageFlagBits::eVertex },
    { Vulkan::ShaderStageBits::eFragment, vk::ShaderStageFlagBits::eFragment },
    { Vulkan::ShaderStageBits::eCompute, vk::ShaderStageFlagBits::eCompute }
};

vk::ShaderStageFlags GetVkShaderStageFlags(ShaderStage stage);

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
