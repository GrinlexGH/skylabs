#pragma once
#include <skylabs/core/render/vertex.hpp>

#include <vulkan/vulkan.hpp>

#include <span>

namespace Vulkan {
class CVertexFormat
{
public:
    explicit CVertexFormat(
        std::span<const CVertexAttribute> attributes,
        std::uint32_t binding = 0,
        vk::VertexInputRate inputRate = vk::VertexInputRate::eVertex
    );
    CVertexFormat(const CVertexFormat&) = delete;
    CVertexFormat(CVertexFormat&&) noexcept = default;
    CVertexFormat& operator=(const CVertexFormat&) = delete;
    CVertexFormat& operator=(CVertexFormat&&) noexcept = default;
    ~CVertexFormat() = default;

    [[nodiscard]] constexpr const std::vector<vk::VertexInputAttributeDescription>& GetAttributeDescriptions() const noexcept { return m_attributes; }
    [[nodiscard]] constexpr const std::vector<vk::VertexInputBindingDescription>& GetBindingDescriptions() const noexcept { return m_bindings; }

private:
    std::vector<vk::VertexInputAttributeDescription> m_attributes;
    std::vector<vk::VertexInputBindingDescription> m_bindings;
};
}
