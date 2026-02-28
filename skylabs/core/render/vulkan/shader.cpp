#include <skylabs/core/render/vulkan/shader.hpp>
#include <skylabs/public/resource_system.hpp>

namespace Vulkan {
vk::ShaderStageFlags GetVkShaderStageFlags(ShaderStage stage) {
    vk::ShaderStageFlags flags;
    for (auto const& [bit, vkBit] : g_shaderStageMap) {
        if (stage & bit) {
            flags |= vkBit;
        }
    }
    return flags;
}

CShader::CShader(const CContext& context, const ShaderStageBits type, const char* name) : m_type(type) {
    vk::ShaderModuleCreateInfo createInfo {};

    const std::vector<char> vertexShaderSource = ResourceSystem::LoadShader(name);

    createInfo.codeSize = vertexShaderSource.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(vertexShaderSource.data());

    m_handle = vk::raii::ShaderModule { *context.Device(), createInfo };

    m_shaderCreateInfo.stage = g_shaderStageMap.at(type);
    m_shaderCreateInfo.module = m_handle;
    m_shaderCreateInfo.pName = "main";
}
}
