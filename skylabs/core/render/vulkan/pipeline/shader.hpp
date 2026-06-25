#pragma once
#include <skylabs/core/render/vulkan/context/context.hpp>

namespace Vulkan {
class CShader
{
public:
    explicit CShader(std::nullptr_t) {}
    explicit CShader(const CContext& context, vk::ShaderStageFlagBits stage, std::string_view name);
    CShader(const CShader&) = delete;
    CShader(CShader&&) noexcept = default;
    CShader& operator=(const CShader&) = delete;
    CShader& operator=(CShader&&) noexcept = default;
    ~CShader() = default;

    [[nodiscard]] const vk::raii::ShaderModule& operator*() const noexcept { return m_handle; }
    [[nodiscard]] const vk::raii::ShaderModule* operator->() const noexcept { return &m_handle; }

    [[nodiscard]] vk::ShaderStageFlagBits Stage() const { return m_stage; }

private:
    vk::raii::ShaderModule m_handle = nullptr;
    vk::ShaderStageFlagBits m_stage = vk::ShaderStageFlagBits::eAll;
};
}
