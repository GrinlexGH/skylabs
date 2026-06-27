#include <skylabs/core/render/vulkan/pipeline/shader.hpp>

import skylabs.pub.filesystem;

namespace Vulkan {
CShader::CShader(const CContext& context, const vk::ShaderStageFlagBits type, const std::string_view name) : m_stage(type) {
    const std::vector<std::uint32_t> code = Filesystem::LoadAsVector32(name);

    vk::ShaderModuleCreateInfo createInfo {};
    createInfo.setCode(code);

    m_handle = vk::raii::ShaderModule { *context.Device(), createInfo };
}
}
