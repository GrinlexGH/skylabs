#include <skylabs/core/render/vulkan/vertex_format.hpp>

namespace {
constexpr vk::Format ToVkFormat(const VertexFormat format) {
    switch (format) {
        case VertexFormat::Float32x2: return vk::Format::eR32G32Sfloat;
        case VertexFormat::Float32x3: return vk::Format::eR32G32B32Sfloat;
    }
    std::unreachable();
}

constexpr std::uint32_t SizeOfFormat(const VertexFormat format) {
    switch (format) {
        case VertexFormat::Float32x2: return 2 * sizeof(float);
        case VertexFormat::Float32x3: return 3 * sizeof(float);
    }
    std::unreachable();
}
}

namespace Vulkan {
CVertexFormat::CVertexFormat(const std::span<const CVertexAttribute> attributes) {
    m_attributes.reserve(attributes.size());
    std::uint32_t i = 0;
    for (const auto& [format, offset] : attributes) {
        vk::VertexInputAttributeDescription attributeDescription {};
        attributeDescription.binding = 0;
        attributeDescription.location = i;
        attributeDescription.format = ToVkFormat(format);
        attributeDescription.offset = offset;
        m_attributes.push_back(attributeDescription);
        ++i;
    }

    std::uint32_t stride = 0;
    for (const auto& [format, offset] : attributes) {
        stride = std::max(stride, offset + SizeOfFormat(format));
    }

    vk::VertexInputBindingDescription bindingDescription {};
    bindingDescription.binding = 0;
    bindingDescription.stride = stride;
    bindingDescription.inputRate = vk::VertexInputRate::eVertex;

    m_bindings.push_back(bindingDescription);
}
}
