#pragma once
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <array>
#include <cstdint>

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
    glm::vec3 m_color;
    glm::vec2 m_texCoord;

    static constexpr auto GetAttributes() -> std::array<CVertexAttribute, 3> {
        return {
            CVertexAttribute { .m_format = VertexFormat::Float32x3, .m_offset = offsetof(CVertex, m_position) },
            CVertexAttribute { .m_format = VertexFormat::Float32x3, .m_offset = offsetof(CVertex, m_color) },
            CVertexAttribute { .m_format = VertexFormat::Float32x2, .m_offset = offsetof(CVertex, m_texCoord) },
        };
    }

    friend constexpr auto operator==(const CVertex& lhs, const CVertex& rhs) -> bool {
        return lhs.m_position == rhs.m_position && lhs.m_color == rhs.m_color && lhs.m_texCoord == rhs.m_texCoord;
    }
};
