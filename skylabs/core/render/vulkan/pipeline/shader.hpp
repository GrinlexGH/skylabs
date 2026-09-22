#pragma once
#include <vulkan/vulkan_raii.hpp>

namespace sk::render::vulkan {
class Shader {
public:
    explicit Shader(std::nullptr_t) { }
    explicit Shader(const vk::raii::Device& device, vk::ShaderStageFlagBits stage,
                    const std::vector<std::uint32_t>& bytecode);
    Shader(const Shader&) = delete;
    Shader(Shader&&) noexcept = default;
    Shader& operator=(const Shader&) = delete;
    Shader& operator=(Shader&&) noexcept = default;
    ~Shader() = default;

    [[nodiscard]] const vk::raii::ShaderModule& operator*() const noexcept { return m_handle; }
    [[nodiscard]] const vk::raii::ShaderModule* operator->() const noexcept { return &m_handle; }

    [[nodiscard]] vk::ShaderStageFlagBits Stage() const { return m_stage; }

private:
    vk::raii::ShaderModule m_handle = nullptr;
    vk::ShaderStageFlagBits m_stage = vk::ShaderStageFlagBits::eAll;
};
}
