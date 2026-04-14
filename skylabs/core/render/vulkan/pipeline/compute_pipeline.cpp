#include <skylabs/core/render/vulkan/pipeline/compute_pipeline.hpp>

#include <ranges>

namespace Vulkan {
CComputePipeline::CComputePipeline(const CContext& context, ComputePipelineCreateInfo options) {
    vk::PipelineShaderStageCreateInfo shader {};
    shader.stage = options.m_shader->Stage();
    shader.module = **options.m_shader;
    shader.pName = "main";

    vk::ComputePipelineCreateInfo pipelineInfo {};
    pipelineInfo.layout = m_layout = options.m_layout;
    pipelineInfo.stage = shader;

    m_handle = vk::raii::Pipeline { *context.Device(), nullptr, pipelineInfo };
}
}
