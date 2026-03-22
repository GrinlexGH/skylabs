#pragma once
#include <skylabs/core/render/vulkan/context/context.hpp>
#include <skylabs/core/render/vulkan/pipeline/shader.hpp>
#include <skylabs/core/render/vertex.hpp>

#include <vector>

namespace Vulkan {
struct ComputePipelineCreateInfo
{
    vk::PipelineLayout m_layout = {};
    const CShader* m_shader = nullptr;
};

class CComputePipeline
{
public:
    explicit CComputePipeline(std::nullptr_t) {}
    explicit CComputePipeline(const CContext& context, ComputePipelineCreateInfo options = {});
    CComputePipeline(const CComputePipeline&) = delete;
    CComputePipeline(CComputePipeline&&) noexcept = default;
    CComputePipeline& operator=(const CComputePipeline&) = delete;
    CComputePipeline& operator=(CComputePipeline&&) noexcept = default;
    ~CComputePipeline() = default;

    [[nodiscard]] const vk::raii::Pipeline& operator*() const noexcept { return m_handle; }
    [[nodiscard]] const vk::raii::Pipeline* operator->() const noexcept { return &m_handle; }

    [[nodiscard]] vk::PipelineLayout Layout() { return m_layout; }

private:
    vk::raii::Pipeline m_handle { nullptr };
    vk::PipelineLayout m_layout { nullptr };
};
}
