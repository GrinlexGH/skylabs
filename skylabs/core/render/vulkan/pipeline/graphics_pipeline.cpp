#include <skylabs/core/render/vulkan/pipeline/graphics_pipeline.hpp>

#include <ranges>

namespace {
constexpr vk::Format ToVkFormat(const VertexFormat format) {
    switch (format) {
        case VertexFormat::Float32x2: return vk::Format::eR32G32Sfloat;
        case VertexFormat::Float32x3: return vk::Format::eR32G32B32Sfloat;
        case VertexFormat::Float32x4: return vk::Format::eR32G32B32A32Sfloat;
    }
    std::unreachable();
}

std::vector<vk::VertexInputAttributeDescription> GenerateAttributeDescriptions(std::span<const Vulkan::VertexBufferBinding> bindings) {
    std::vector<vk::VertexInputAttributeDescription> descriptions;
    descriptions.reserve(bindings.size());

    for (const auto& [description, attributes] : bindings) {
        for (std::uint32_t i = 0; const auto& [format, offset] : attributes) {
            vk::VertexInputAttributeDescription attributeDescription {};
            attributeDescription.binding = description.binding;
            attributeDescription.location = i;
            attributeDescription.format = ToVkFormat(format);
            attributeDescription.offset = offset;
            descriptions.push_back(attributeDescription);
            ++i;
        }
    }

    return descriptions;
}
}

namespace Vulkan {
CGraphicsPipeline::CGraphicsPipeline(const CContext& context, GraphicsPipelineCreateInfo options) {
    const std::vector vertexAttributeDescriptions = GenerateAttributeDescriptions(options.m_vertexBindings);
    const std::vector vertexBindingDescriptions =
        std::views::transform(options.m_vertexBindings, [](Vulkan::VertexBufferBinding& binding) { return binding.m_description; })
        | std::ranges::to<std::vector>();

    vk::PipelineVertexInputStateCreateInfo vertexInputInfo {};
    vertexInputInfo.vertexBindingDescriptionCount = static_cast<std::uint32_t>(vertexBindingDescriptions.size());
    vertexInputInfo.pVertexBindingDescriptions = vertexBindingDescriptions.data();
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<std::uint32_t>(vertexAttributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = vertexAttributeDescriptions.data();

    vk::PipelineInputAssemblyStateCreateInfo inputAssembly {};
    inputAssembly.topology = options.m_primitiveTopology;
    inputAssembly.primitiveRestartEnable = vk::False;

    vk::PipelineViewportStateCreateInfo viewportState {};
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    vk::PipelineRasterizationStateCreateInfo rasterizer {};
    rasterizer.depthClampEnable = vk::False;
    rasterizer.rasterizerDiscardEnable = vk::False;
    rasterizer.polygonMode = vk::PolygonMode::eFill;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = vk::CullModeFlagBits::eNone;
    rasterizer.frontFace = vk::FrontFace::eClockwise;
    rasterizer.depthBiasEnable = vk::False;

    vk::PipelineMultisampleStateCreateInfo multisampling {};
    multisampling.sampleShadingEnable = vk::False;
    multisampling.rasterizationSamples = options.m_sampling;

    vk::PipelineColorBlendAttachmentState colorBlendAttachment {};
    colorBlendAttachment.colorWriteMask =
        vk::ColorComponentFlagBits::eR
        | vk::ColorComponentFlagBits::eG
        | vk::ColorComponentFlagBits::eB
        | vk::ColorComponentFlagBits::eA;
    colorBlendAttachment.blendEnable = vk::True;
    colorBlendAttachment.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
    colorBlendAttachment.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
    colorBlendAttachment.colorBlendOp = vk::BlendOp::eAdd;

    colorBlendAttachment.srcAlphaBlendFactor = vk::BlendFactor::eOne;
    colorBlendAttachment.dstAlphaBlendFactor = vk::BlendFactor::eZero;
    colorBlendAttachment.alphaBlendOp = vk::BlendOp::eAdd;

    vk::PipelineColorBlendStateCreateInfo colorBlending {};
    colorBlending.logicOpEnable = vk::False;
    colorBlending.logicOp = vk::LogicOp::eCopy;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    constexpr std::array dynamicStates = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor,
        vk::DynamicState::eDepthBias,
        vk::DynamicState::eDepthBiasEnable
    };
    vk::PipelineDynamicStateCreateInfo dynamicState {};
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    vk::PipelineDepthStencilStateCreateInfo depthStencil {};
    depthStencil.depthTestEnable = vk::True;
    depthStencil.depthWriteEnable = vk::True;
    depthStencil.depthCompareOp = vk::CompareOp::eGreater;
    depthStencil.depthBoundsTestEnable = vk::False;
    depthStencil.stencilTestEnable = vk::False;

    std::vector<vk::PipelineShaderStageCreateInfo> shaderCreateInfo {};
    shaderCreateInfo.reserve(options.m_shaders.size());
    for (std::size_t i = 0; i < options.m_shaders.size(); ++i) {
        vk::PipelineShaderStageCreateInfo ci {};
        ci.stage = options.m_shaders[i]->Stage();
        ci.module = **options.m_shaders[i];
        ci.pName = "main";
        shaderCreateInfo.push_back(ci);
    }

    vk::GraphicsPipelineCreateInfo pipelineInfo {};
    pipelineInfo.stageCount = static_cast<std::uint32_t>(shaderCreateInfo.size());
    pipelineInfo.pStages = shaderCreateInfo.data();
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.layout = m_layout = options.m_layout;
    pipelineInfo.renderPass = nullptr;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = nullptr;
    pipelineInfo.pNext = &options.m_renderingInfo;

    m_handle = vk::raii::Pipeline { (*context.Device()), nullptr, pipelineInfo };
}
}
