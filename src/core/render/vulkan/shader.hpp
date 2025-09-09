#pragma once
#include "context/context.hpp"

namespace Vulkan {
class CShader
{
public:
    enum class Type : std::uint8_t
    {
        eVertex = 0,
        eFragment,
        eCount,
    };

    explicit CShader(const CContext* context, Type type, const char* name);
    CShader(const CShader&) = delete;
    CShader(CShader&&) noexcept = default;
    CShader& operator=(const CShader&) = delete;
    CShader& operator=(CShader&&) noexcept = default;
    ~CShader() = default;

    [[nodiscard]] auto GetHandle() -> const vk::raii::ShaderModule& { return m_handle; }
    [[nodiscard]] auto GetPipelineShaderCreateInfo() const -> vk::PipelineShaderStageCreateInfo { return m_shaderCreateInfo; }

private:
    constexpr static std::array ShaderStageTable = {
        vk::ShaderStageFlagBits::eVertex,   // Type::eVertex
        vk::ShaderStageFlagBits::eFragment, // Type::eFragment
    };

    constexpr static auto ToVkStage(Type type) -> vk::ShaderStageFlagBits {
        static_assert(
            ShaderStageTable.size() == static_cast<std::size_t>(Type::eCount),
            "ShaderStageTable size must match Vulkan::CShader::Type::eCount"
        );
        return ShaderStageTable[static_cast<std::size_t>(type)];
    }

    vk::raii::ShaderModule m_handle = nullptr;
    vk::PipelineShaderStageCreateInfo m_shaderCreateInfo;
    Type m_type;
};
}
