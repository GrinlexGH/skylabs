#include "skylabs/core/render/vulkan/pipeline/compute_pipeline.hpp"

namespace sk::render::vulkan {
ComputePipeline::ComputePipeline(const vk::raii::Device& device,
                                 const ComputePipelineCreateInfo& options) {
    vk::PipelineShaderStageCreateInfo shader { };
    shader.stage = options.shader->Stage();
    shader.module = **options.shader;
    shader.pName = "main";

    vk::ComputePipelineCreateInfo pipelineInfo { };
    pipelineInfo.layout = m_layout = options.layout;
    pipelineInfo.stage = shader;

    m_handle = vk::raii::Pipeline { device, nullptr, pipelineInfo };
}
}
