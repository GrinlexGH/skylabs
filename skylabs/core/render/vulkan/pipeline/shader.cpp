#include <skylabs/core/render/vulkan/pipeline/shader.hpp>
#include <skylabs/public/resource_system.hpp>

namespace Vulkan {
CShader::CShader(const CContext& context, const vk::ShaderStageFlagBits type, const char* name) : m_stage(type) {
    const std::vector<char> vertexShaderSource = ResourceSystem::LoadShader(name);

    vk::ShaderModuleCreateInfo createInfo {};
    createInfo.codeSize = vertexShaderSource.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(vertexShaderSource.data());

    m_handle = vk::raii::ShaderModule { *context.Device(), createInfo };
}
}
