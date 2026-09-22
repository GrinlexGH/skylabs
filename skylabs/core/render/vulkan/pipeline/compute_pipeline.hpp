#pragma once
#include "skylabs/core/render/vulkan/pipeline/shader.hpp"

namespace sk::render::vulkan {
struct ComputePipelineCreateInfo {
    vk::PipelineLayout layout = { };
    const Shader* shader = nullptr;
};

class ComputePipeline {
public:
    explicit ComputePipeline(std::nullptr_t) { }
    explicit ComputePipeline(const vk::raii::Device& device,
                             const ComputePipelineCreateInfo& options = { });
    ComputePipeline(const ComputePipeline&) = delete;
    ComputePipeline(ComputePipeline&&) noexcept = default;
    ComputePipeline& operator=(const ComputePipeline&) = delete;
    ComputePipeline& operator=(ComputePipeline&&) noexcept = default;
    ~ComputePipeline() = default;

    [[nodiscard]] const vk::raii::Pipeline& operator*() const noexcept { return m_handle; }
    [[nodiscard]] const vk::raii::Pipeline* operator->() const noexcept { return &m_handle; }

    [[nodiscard]] vk::PipelineLayout Layout() const { return m_layout; }

private:
    vk::raii::Pipeline m_handle { nullptr };
    vk::PipelineLayout m_layout { nullptr };
};
}
