#include <skylabs/core/render/vulkan/pipeline/shader.hpp>
#include <skylabs/public/filesystem.hpp>

namespace Vulkan {
CShader::CShader(const vk::raii::Device& device, const vk::ShaderStageFlagBits type, const std::string_view name) : m_stage(type) {
    const std::vector<std::uint32_t> code = Filesystem::LoadAsVector32(name);

    vk::ShaderModuleCreateInfo createInfo {};
    createInfo.setCode(code);

    m_handle = vk::raii::ShaderModule { device, createInfo };
}
}
