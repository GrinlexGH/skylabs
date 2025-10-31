#include <skylabs/core/render/vulkan/shader.hpp>

#include <skylabs/public/resource_system.hpp>

namespace Vulkan {
CShader::CShader(const CContext& context, const Type type, const char* name) {
    vk::ShaderModuleCreateInfo createInfo {};

    const std::vector<char> vertexShaderSource = ResourceSystem::LoadShader(name);

    createInfo.codeSize = vertexShaderSource.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(vertexShaderSource.data());

    m_handle = (*context.GetDevice()).createShaderModule(createInfo);

    m_shaderCreateInfo.stage = ToVkStage(type);
    m_shaderCreateInfo.module = m_handle;
    m_shaderCreateInfo.pName = "main";
}
}
