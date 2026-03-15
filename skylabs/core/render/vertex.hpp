#pragma once
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <array>

enum class VertexFormat : std::uint8_t {
    Float32x2,
    Float32x3,
};

struct CVertexAttribute
{
    VertexFormat m_format;
    std::uint32_t m_offset;
};

struct CVertex
{
    glm::vec3 m_position;
    glm::vec2 m_texCoord;

    static constexpr std::array<CVertexAttribute, 2> GetAttributes() {
        return {
            CVertexAttribute { .m_format = VertexFormat::Float32x3, .m_offset = offsetof(CVertex, m_position) },
            CVertexAttribute { .m_format = VertexFormat::Float32x2, .m_offset = offsetof(CVertex, m_texCoord) },
        };
    }

    friend constexpr bool operator==(const CVertex& lhs, const CVertex& rhs) {
        return lhs.m_position == rhs.m_position && lhs.m_texCoord == rhs.m_texCoord;
    }
};
