#include <skylabs/core/render/vulkan/shader.hpp>

#include <skylabs/public/resource_system.hpp>

namespace {
constexpr std::array ShaderStageTable = {
    vk::ShaderStageFlagBits::eVertex,   // Type::eVertex
    vk::ShaderStageFlagBits::eFragment, // Type::eFragment
};

static_assert(
    ShaderStageTable.size() == static_cast<std::size_t>(Vulkan::CShader::Type::eCount),
    "ShaderStageTable size must match Vulkan::CShader::Type::eCount"
);

constexpr auto ToVkStage(Vulkan::CShader::Type type) -> vk::ShaderStageFlagBits {
    return ShaderStageTable[static_cast<std::size_t>(type)];
}
}

namespace Vulkan {
CShader::CShader(const CContext& context, const Type type, const char* name) {
    vk::ShaderModuleCreateInfo createInfo {};

    const std::vector<char> vertexShaderSource = ResourceSystem::LoadShader(name);

    createInfo.codeSize = vertexShaderSource.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(vertexShaderSource.data());

    m_handle = vk::raii::ShaderModule { *context.Device(), createInfo };

    m_shaderCreateInfo.stage = ToVkStage(type);
    m_shaderCreateInfo.module = m_handle;
    m_shaderCreateInfo.pName = "main";
}
}
