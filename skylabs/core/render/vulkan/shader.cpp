#include <skylabs/core/render/vulkan/shader.hpp>
#include <skylabs/public/resource_system.hpp>

#include <frozen/map.h>

namespace {
constexpr frozen::map<Vulkan::ShaderStageBits, vk::ShaderStageFlagBits, 3> g_shaderStageMap
{
    { Vulkan::ShaderStageBits::eVertex, vk::ShaderStageFlagBits::eVertex },
    { Vulkan::ShaderStageBits::eFragment, vk::ShaderStageFlagBits::eFragment },
    { Vulkan::ShaderStageBits::eCompute, vk::ShaderStageFlagBits::eCompute }
};
}

namespace Vulkan {
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
