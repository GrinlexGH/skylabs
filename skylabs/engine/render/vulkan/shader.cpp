#include "skylabs/engine/render/vulkan/shader.hpp"

namespace sk::render::vulkan {
Shader::Shader(const vk::raii::Device& device, const vk::ShaderStageFlagBits stage,
               const std::vector<std::uint32_t>& bytecode)
    : m_stage(stage) {
    vk::ShaderModuleCreateInfo createInfo { };
    createInfo.setCode(bytecode);

    m_handle = vk::raii::ShaderModule { device, createInfo };
}
}
