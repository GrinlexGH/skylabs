#include <skylabs/core/render/vulkan/pipeline/graphics_pipeline.hpp>

#include <frozen/map.h>
#include <ranges>

namespace {
constexpr frozen::map<VertexFormat, vk::Format, 3> g_vertexFormat = {
    { VertexFormat::Float32x2, vk::Format::eR32G32Sfloat },
    { VertexFormat::Float32x3, vk::Format::eR32G32B32Sfloat },
    { VertexFormat::Float32x4, vk::Format::eR32G32B32A32Sfloat },
};

constexpr vk::Format ToVkFormat(const VertexFormat format) {
    if (!g_vertexFormat.contains(format)) {
        assert(false && "Unsupported vertex format");
        return vk::Format::eR8G8B8A8Snorm;
    }

    return g_vertexFormat.at(format);
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
    vertexInputInfo.setVertexBindingDescriptions(vertexBindingDescriptions);
    vertexInputInfo.setVertexAttributeDescriptions(vertexAttributeDescriptions);

    vk::PipelineInputAssemblyStateCreateInfo inputAssembly {};
    inputAssembly.topology = options.m_primitiveTopology;
    inputAssembly.primitiveRestartEnable = vk::False;

    vk::PipelineViewportStateCreateInfo viewportState {};
    viewportState.viewportCount = 1;
    viewportState.pViewports = nullptr;
    viewportState.scissorCount = 1;
    viewportState.pScissors = nullptr;

    vk::PipelineRasterizationStateCreateInfo rasterizer {};
    rasterizer.depthClampEnable = vk::False;
    rasterizer.rasterizerDiscardEnable = vk::False;
    rasterizer.polygonMode = vk::PolygonMode::eFill;
    rasterizer.cullMode = vk::CullModeFlagBits::eNone;
    rasterizer.frontFace = vk::FrontFace::eClockwise;
    rasterizer.depthBiasEnable = vk::False;
    rasterizer.depthBiasConstantFactor = 0.0f;
    rasterizer.depthBiasClamp = 0.0f;
    rasterizer.depthBiasSlopeFactor = 0.0f;
    rasterizer.lineWidth = 1.0f;

    vk::PipelineMultisampleStateCreateInfo multisampling {};
    multisampling.rasterizationSamples = options.m_sampling;
    multisampling.sampleShadingEnable = vk::False;
    multisampling.minSampleShading = 0.0f;
    multisampling.pSampleMask = nullptr;
    multisampling.alphaToCoverageEnable = vk::False;
    multisampling.alphaToOneEnable = vk::False;

    // TODO: bleding settings
    vk::PipelineColorBlendAttachmentState colorBlendAttachment {};
    colorBlendAttachment.blendEnable = vk::True;
    colorBlendAttachment.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
    colorBlendAttachment.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
    colorBlendAttachment.colorBlendOp = vk::BlendOp::eAdd;
    colorBlendAttachment.srcAlphaBlendFactor = vk::BlendFactor::eOne;
    colorBlendAttachment.dstAlphaBlendFactor = vk::BlendFactor::eZero;
    colorBlendAttachment.alphaBlendOp = vk::BlendOp::eAdd;
    colorBlendAttachment.colorWriteMask =
        vk::ColorComponentFlagBits::eR
        | vk::ColorComponentFlagBits::eG
        | vk::ColorComponentFlagBits::eB
        | vk::ColorComponentFlagBits::eA;

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
    };
    vk::PipelineDynamicStateCreateInfo dynamicState {};
    dynamicState.setDynamicStates(dynamicStates);

    vk::PipelineDepthStencilStateCreateInfo depthStencil {};
    depthStencil.depthTestEnable = vk::True;
    depthStencil.depthWriteEnable = vk::True;
    depthStencil.depthCompareOp = vk::CompareOp::eGreater;
    depthStencil.depthBoundsTestEnable = vk::False;
    depthStencil.stencilTestEnable = vk::False;
    depthStencil.front = vk::StencilOpState { };
    depthStencil.back = vk::StencilOpState { };
    depthStencil.minDepthBounds = 0.0f;
    depthStencil.maxDepthBounds = 0.0f;

    std::vector<vk::PipelineShaderStageCreateInfo> shaderCreateInfo {};
    shaderCreateInfo.reserve(options.m_shaders.size());
    for (auto & m_shader : options.m_shaders) {
        vk::PipelineShaderStageCreateInfo ci {};
        ci.stage = m_shader->Stage();
        ci.module = **m_shader;
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
