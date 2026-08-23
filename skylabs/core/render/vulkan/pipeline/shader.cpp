#include <skylabs/core/render/vulkan/pipeline/shader.hpp>
#include <skylabs/public/filesystem.hpp>

namespace Vulkan {
CShader::CShader(const vk::raii::Device& device, const vk::ShaderStageFlagBits type, const std::vector<std::uint32_t>& bytecode) : m_stage(type) {
    vk::ShaderModuleCreateInfo createInfo {};
    createInfo.setCode(bytecode);

    m_handle = vk::raii::ShaderModule { device, createInfo };
}
}
